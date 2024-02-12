
#include <iostream>
#include <memory>

#include <boost/mysql/error_with_diagnostics.hpp>
#include <boost/mysql/handshake_params.hpp>
#include <boost/mysql/results.hpp>
#include <boost/system/system_error.hpp>
#include <boost/core/span.hpp>

#include <fmt/core.h>

#include "utils/DBConnection.h"

DBConnection::DBConnection() : _ssl_ctx(boost::asio::ssl::context::tls_client), _conn(_ctx, _ssl_ctx) {}

DBConnection::~DBConnection() {
    _conn.close();
}

bool DBConnection::connect() {
    static bool logged = false;
    try {
        boost::asio::ip::tcp::resolver resolver(_ctx.get_executor());
        auto endpoints = resolver.resolve("127.0.0.1", boost::mysql::default_port_string);
        boost::mysql::handshake_params params("root", "", "test", boost::mysql::handshake_params::default_collation, boost::mysql::ssl_mode::enable);

        if (!logged) std::cout << "Connecting to mysql server at " << endpoints.begin()->endpoint() << " ... ";

        _conn.connect(*endpoints.begin(), params);
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
        return false;
    }
    catch (const std::exception& err) {
        std::cerr << "Error: " << err.what() << std::endl;
        return false;
    }
    if (!logged) { logged = true; printf("Connected\n"); }
    return true;
}

bool DBConnection::query(const char* sql, boost::mysql::results &result) {
    try {
        _conn.execute(sql, result);
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
        return false;
    }
    catch (const std::exception& err) {
        std::cerr << "Error: " << err.what() << std::endl;
        return false;
    }
    return true;
}

void DBConnection::createTables() {

    printf("Checking tables ... ");

    boost::mysql::results r;

    query("CREATE TABLE IF NOT EXISTS students (\
        id INT AUTO_INCREMENT PRIMARY KEY\
    )", r);
    query("CREATE TABLE IF NOT EXISTS facial_data (\
        student_id INT, \
        device_id INT, \
        facial_features BLOB(512), \
        CONSTRAINT FK_student FOREIGN KEY (student_id) REFERENCES students(id)\
    )", r);
    query("CREATE TABLE IF NOT EXISTS schedules (\
        student_id INT, \
        period INT, \
        room_id INT, \
        CONSTRAINT FK_student2 FOREIGN KEY (student_id) REFERENCES students(id)\
    )", r);
    //         
    query("CREATE TABLE IF NOT EXISTS attendance (\
        day DATE DEFAULT CURRENT_DATE, \
        room_id INT, \
        period INT, \
        student_id INT, \
        status ENUM('PRESENT', 'ABSENT'), \
        CONSTRAINT FK_student3 FOREIGN KEY(student_id) REFERENCES students(id)\
    )", r);

    const int face_bytes = FACE_VEC_SIZE * 4;
    const int face_cov_bytes = FACE_VEC_SIZE * FACE_VEC_SIZE * 4;
    // needs expected dts between devs for periods
    query(fmt::format("CREATE TABLE IF NOT EXISTS long_term_states (\
        id INT AUTO_INCREMENT PRIMARY KEY, \
        mean_facial_features BLOB({}), \
        cov_facial_features BLOB({}), \
        student_id INT, \
        CONSTRAINT FK_student4 FOREIGN KEY (student_id) REFERENCES students(id) \
    )", face_bytes, face_cov_bytes).c_str(), r);
    query(fmt::format("CREATE TABLE IF NOT EXISTS short_term_states (\
        id INT AUTO_INCREMENT PRIMARY KEY NOT NULL, \
        mean_facial_features BLOB({}) NOT NULL, \
        cov_facial_features BLOB({}), \
        update_count INT, \
        last_update_device_id INT NOT NULL, \
        last_update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP, \
        expected_next_update_device_id INT, \
        expected_next_update_time TIMESTAMP NULL DEFAULT NULL, \
        expected_next_update_time_variance FLOAT(0), \
        long_term_state_key INT, \
        CONSTRAINT FK_lts FOREIGN KEY (long_term_state_key) REFERENCES long_term_states(id) \
    )", face_bytes, face_cov_bytes).c_str(), r);
    query(fmt::format("CREATE TABLE IF NOT EXISTS updates (\
        id INT AUTO_INCREMENT PRIMARY KEY, \
        device_id INT, \
        facial_features BLOB({}), \
        time TIMESTAMP DEFAULT CURRENT_TIMESTAMP, \
        period INT, \
        short_term_state_id INT, \
        CONSTRAINT FK_sts FOREIGN KEY (short_term_state_id) REFERENCES short_term_states(id)\
    )", face_bytes).c_str(), r);

    printf("Done\n");

}

void DBConnection::clearTables() {
    printf("Clearing tables ... ");
    boost::mysql::results r;
	query("SET FOREIGN_KEY_CHECKS = 0", r);
    query("TRUNCATE attendance", r);
    query("TRUNCATE facial_data", r);
    query("TRUNCATE long_term_states", r);
    query("TRUNCATE short_term_states", r);
    query("TRUNCATE students", r);
    query("TRUNCATE updates", r);
    query("TRUNCATE schedules", r);
	query("SET FOREIGN_KEY_CHECKS = 1", r);
    printf("Done\n");
}

void DBConnection::getEntities(std::vector<EntityPtr>& vec) {
    printf("Loading entities ... ");
    try {
        boost::mysql::results result;
        _conn.execute("SELECT id FROM students", result);
        if (!result.empty()) {
            for (const boost::mysql::row_view& row : result.rows()) {
                int id = row.at(0).as_int64();

                std::vector<int> schedule;
                //boost::mysql::results scheduleResult;
                //_conn.execute(_conn.prepare_statement("SELECT room_id FROM schedules WHERE student_id=? ORDER BY period ASC").bind(id), scheduleResult);
                //for (const boost::mysql::row_view& scheduleRow : scheduleResult.rows()) {
                //    schedule.push_back(scheduleRow[0].as_int64());
                //}
                vec.push_back(EntityPtr(new Entity(id, schedule)));
            }
        }
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
    printf("Done\n");
}

bool DBConnection::getEntityFeatures(EntityPtr entity, int devId) {
    try {
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "SELECT facial_features FROM facial_data WHERE student_id=? AND device_id=?"
            ).bind(entity->id, devId), result);
        if (result.rows().size() > 0) {
            entity->setFacialFeatures(result.rows()[0][0].as_blob());
            return true;
        }
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
    return false;
}


void DBConnection::pushUpdate(int devId, const boost::span<UCHAR> facialFeatures) {
    fmt::print("Pushing update for device {} ... ", devId);
    try {
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement("INSERT INTO updates (device_id, facial_features) VALUES(?, ?)").bind(devId, facialFeatures), result);
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
    printf("Done\n");
}

void DBConnection::getNewUpdates(std::vector<UpdatePtr>& updates) {
    //printf("Fetching updates ... ");
    boost::mysql::results result;
    query("SELECT id, device_id, facial_features FROM updates WHERE short_term_state_id IS NULL ORDER BY time ASC", result);
    if (!result.empty()) {
        for (const boost::mysql::row_view& row : result.rows()) {
            updates.push_back(UpdatePtr(new Update(row[0].as_int64(), row[1].as_int64(), row[2].as_blob())));
        }
    }
    //printf("Done\n");
}

void DBConnection::updateUpdate(UpdatePtr update) {
    try {
        fmt::print("Updating update {} with sts id {}\n", update->id, update->shortTermStateId);
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "UPDATE updates SET short_term_state_id=? WHERE id=?"
        ).bind(update->shortTermStateId, update->id), result);
        printf("Done\n");
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

void DBConnection::removePreviousUpdates(UpdatePtr update) {
    try {
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "DELETE FROM updates WHERE short_term_state_id=? AND period IS NULL"
        ).bind(update->shortTermStateId), result);
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

void DBConnection::setUpdatesPeriod(int period) {
    try {
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "UPDATE updates SET period=? WHERE period IS NULL"
        ).bind(period), result);
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

void DBConnection::getUpdatesPath(int stsId, std::vector<int>& devPath) {
    try {
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "SELECT device_id FROM updates WHERE short_term_state_id=? ORDER BY period"
        ).bind(stsId), result);
        for (const boost::mysql::row_view& row : result.rows()) {
            devPath.push_back(row[0].as_int64());
        }
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

void DBConnection::clearUpdates() {
    printf("Clearing updates ... ");
    boost::mysql::results result;
    query("DELETE FROM updates", result);
    printf("Done\n");
}

LongTermStatePtr DBConnection::getLongTermState(int id) {
    try {
        printf("Fetching long term state ... ");
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "SELECT id, mean_facial_features, cov_facial_features, student_id FROM long_term_states WHERE id=?"
        ).bind(id), result);
        printf("Done\n");
        auto row = result.rows()[0];
        if (row[3].is_int64()) {
            return LongTermStatePtr(new LongTermState(row[0].as_int64(), row[1].as_blob(), row[2].as_blob(), row[3].as_int64()));
        }
        return LongTermStatePtr(new LongTermState(row[0].as_int64(), row[1].as_blob(), row[2].as_blob()));
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
    return nullptr;
}

void DBConnection::getLongTermStates(std::vector<LongTermStatePtr> &states) {
    printf("Fetching long term states ... ");
    boost::mysql::results result;
    query("SELECT id, mean_facial_features, cov_facial_features FROM long_term_states ORDER BY id ASC", result);
    if (!result.empty()) {
        for (const boost::mysql::row_view& row : result.rows()) {
            if (row[3].is_int64()) {
                states.push_back(LongTermStatePtr(new LongTermState(row[0].as_int64(), row[1].as_blob(), row[2].as_blob(), row[3].as_int64())));
            }
            states.push_back(LongTermStatePtr(new LongTermState(row[0].as_int64(), row[1].as_blob(), row[2].as_blob())));
        }
    }
    printf("Done\n");
}

int DBConnection::createLongTermState(ShortTermStatePtr sts) {
    try {
        printf("Creating long term state ... ");
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "INSERT INTO long_term_states (mean_facial_features, cov_facial_features) VALUES(?,?)"
        ).bind(sts->getFacialFeatures(), sts->getFacialFeaturesCovSpan()), result);
        query("SELECT LAST_INSERT_ID()", result);
        printf("Done\n");
        return result.rows()[0][0].as_uint64();
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
    return -1;
}

void DBConnection::updateLongTermState(LongTermStatePtr lts) {
    try {
        fmt::print("Updating long term state {} ... ", lts->id);
        boost::mysql::results result;
        if (lts->studentId == -1) {
            _conn.execute(_conn.prepare_statement(
                "UPDATE long_term_states SET mean_facial_features=?, cov_facial_features=? WHERE id=?"
            ).bind(lts->getFacialFeatures(), lts->getFacialFeaturesCovSpan(), lts->id), result);
        } else {
            _conn.execute(_conn.prepare_statement(
                "UPDATE long_term_states SET mean_facial_features=?, cov_facial_features=?, student_id=? WHERE id=?"
            ).bind(lts->getFacialFeatures(), lts->getFacialFeaturesCovSpan(), lts->studentId, lts->id), result);
        }
        printf("Done\n");
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

void DBConnection::setLongTermStateStudent(LongTermStatePtr lts) {
    try {
        fmt::print("Setting {} lts to student {} ... ", lts->id, lts->studentId);
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "UPDATE long_term_states SET student_id=? WHERE id=?"
        ).bind(lts->studentId, lts->id), result);
        printf("Done\n");
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

void DBConnection::getShortTermStates(std::vector<ShortTermStatePtr> &states, bool small) {
    boost::mysql::results result;
    if (!small) {
        printf("Fetching short term states ... ");
        query("SELECT id, mean_facial_features, cov_facial_features, update_count, last_update_device_id, long_term_state_key FROM short_term_states ORDER BY id ASC", result);
        if (!result.empty()) {
            for (const boost::mysql::row_view& row : result.rows()) {
                if (row[5].is_int64()) {
                    states.push_back(ShortTermStatePtr(new ShortTermState(row[0].as_int64(), row[1].as_blob(), row[2].as_blob(), row[3].as_int64(), row[4].as_int64(), row[5].as_int64())));
                } else {
                    states.push_back(ShortTermStatePtr(new ShortTermState(row[0].as_int64(), row[1].as_blob(), row[2].as_blob(), row[3].as_int64(), row[4].as_int64())));
                }
            }
        }
        printf("Done\n");
    } else {
        query("SELECT id, update_count, last_update_device_id, long_term_state_key FROM short_term_states ORDER BY id ASC", result);
        if (!result.empty()) {
            for (const boost::mysql::row_view& row : result.rows()) {
                if (row[3].is_int64()) {
                    states.push_back(ShortTermStatePtr(new ShortTermState(row[0].as_int64(), row[1].as_int64(), row[2].as_int64(), row[3].as_int64())));
                } else {
                    states.push_back(ShortTermStatePtr(new ShortTermState(row[0].as_int64(), row[1].as_int64(), row[2].as_int64())));
                }
            }
        }

    }
}

void DBConnection::getLinkedLongTermStateIds(std::set<int>& ids) {
    printf("Fetching linked long term state ids ... ");
    boost::mysql::results result;
    query("SELECT long_term_state_key FROM short_term_states WHERE long_term_state_key IS NOT NULL", result);
    if (!result.empty()) {
        for (const boost::mysql::row_view& row : result.rows()) {
            ids.insert(row[0].as_int64());
        }
    }
    printf("Done\n");
}

ShortTermStatePtr DBConnection::getLastShortTermState(int ltsId) {
    fmt::print("Fetching last short term state with lts {} ... ", ltsId);
    boost::mysql::results result;
    _conn.execute(_conn.prepare_statement(
        "SELECT id, mean_facial_features, cov_facial_features, update_count, last_update_device_id, long_term_state_key \
        FROM short_term_states WHERE long_term_state_key=? ORDER BY last_update_time DESC LIMIT 1"
        ).bind(ltsId), result);
    if (!result.empty()) {
        for (const boost::mysql::row_view& row : result.rows()) {
            printf("Done\n");
            return ShortTermStatePtr(new ShortTermState(row[0].as_int64(), row[1].as_blob(), row[2].as_blob(), row[3].as_int64(), row[4].as_int64(), row[5].as_int64()));
        }
    }
    return nullptr;
}

int DBConnection::createShortTermState(UpdateCPtr update, LongTermStatePtr ltState) {
    try {
        printf("Creating short term state ... ");
        boost::mysql::results result;
        if (ltState != nullptr) {
            _conn.execute(_conn.prepare_statement(
                "INSERT INTO short_term_states (mean_facial_features, cov_facial_features, update_count, last_update_device_id, long_term_state_key) VALUES(?,?,?,?,?)"
            ).bind(update->getFacialFeatures(), update->getFacialFeaturesCovSpan(), 1, update->deviceId, ltState->id), result);
        } else {
            _conn.execute(_conn.prepare_statement(
                "INSERT INTO short_term_states (mean_facial_features, cov_facial_features, update_count, last_update_device_id) VALUES(?,?,?,?)"
            ).bind(update->getFacialFeatures(), update->getFacialFeaturesCovSpan(), 1, update->deviceId), result);
        }
        query("SELECT LAST_INSERT_ID()", result);
        printf("Done\n");
        return result.rows()[0][0].as_uint64();
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
    return -1;
}

void DBConnection::updateShortTermState(ShortTermStatePtr state) {
    try {
        printf("Updating short term state ... ");
        boost::mysql::results result;
        if (state->longTermStateKey != -1) {
            _conn.execute(_conn.prepare_statement(
                "UPDATE short_term_states SET mean_facial_features=?, cov_facial_features=?, update_count=?, last_update_device_id=?, long_term_state_key=? WHERE id=?"
            ).bind(state->getFacialFeatures(), state->getFacialFeaturesCovSpan(), state->updateCount, state->lastUpdateDeviceId, state->longTermStateKey, state->id), result);
        } else {
            _conn.execute(_conn.prepare_statement(
                "UPDATE short_term_states SET mean_facial_features=?, cov_facial_features=?, update_count=?, last_update_device_id=? WHERE id=?"
            ).bind(state->getFacialFeatures(), state->getFacialFeaturesCovSpan(), state->updateCount, state->lastUpdateDeviceId, state->id), result);
        }
        printf("Done\n");
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

void DBConnection::clearShortTermStates() {
    printf("Clearing short term states ... ");
    boost::mysql::results result;
    query("DELETE FROM short_term_states", result);
    printf("Done\n");

}

int DBConnection::getScheduledRoom(int studentId, int period) {
    try {
        fmt::print("Gettting room for student {} in period {} ... ", studentId, period);
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "SELECT room_id FROM schedules WHERE student_id=? AND period=?"
        ).bind(studentId, period), result);
        printf("Done\n");
        return result.rows()[0][0].as_int64();
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
    return -1;
}

void DBConnection::getSchedules(std::vector<Schedule>& schedules) {
    try {
        printf("Getting schedules ... ");
        boost::mysql::results result;
        query("SELECT student_id, room_id FROM schedules ORDER BY student_id, period", result);
        std::vector<int> rooms;
        int student = -1;
        for (const boost::mysql::row_view& row : result.rows()) {
            if (student != row[0].as_int64() && student != -1) {
                schedules.push_back(Schedule{student, rooms});
                rooms.clear();
            }
            student = row[0].as_int64();
            rooms.push_back(row[1].as_int64());
        }
        if (student != -1) schedules.push_back(Schedule{ student, rooms });
        printf("Done\n");
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

void DBConnection::addToSchedule(int studentId, int period, int roomId) {
    try {
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "INSERT INTO schedules (student_id, period, room_id) VALUES (?,?,?)"
        ).bind(studentId, period, roomId), result);
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

void DBConnection::setAttendance(int room, int period, int studentId, AttendanceStatus status) {
    try {
        std::string statusS = status == AttendanceStatus::ABSENT ? "ABSENT" : "PRESENT";
        fmt::print("Setting attendance for student {} in period {} to {} ... ", studentId, period, statusS);
        boost::mysql::results result;
        _conn.execute(_conn.prepare_statement(
            "INSERT INTO attendance (room_id, period, student_id, status) VALUES (?,?,?,?)"
        ).bind(room, period, studentId, statusS), result);
        printf("Done\n");
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}

int DBConnection::getPeriod() {
    boost::mysql::results result;
    query("SELECT period FROM updates WHERE period IS NOT NULL ORDER BY time DESC LIMIT 1", result);
    if (result.rows().size() > 0) {
        return result.rows()[0][0].as_int64();
    }
    return -1;
}
int DBConnection::addStudent() {
    try {
        boost::mysql::results result;
        query("INSERT INTO students () VALUES()", result);
        query("SELECT LAST_INSERT_ID()", result);
        return result.rows()[0][0].as_uint64();
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
    return -1;
}

void DBConnection::pushStudentData(UpdatePtr data, int studentId) {
    try {
        boost::mysql::results result;
        _conn.execute(
            _conn.prepare_statement("INSERT INTO facial_data (student_id, device_id, facial_features) VALUES(?, ?, ?)")
            .bind(studentId, data->deviceId, data->getFacialFeatures()), result);
    }
    catch (const boost::mysql::error_with_diagnostics& err) {
        std::cerr << "Error: " << err.what() << '\n'
            << "Server diagnostics: " << err.get_diagnostics().server_message() << std::endl;
    }
}