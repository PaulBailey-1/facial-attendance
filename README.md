This project developed algorithms and an architecture that could be used to automate taking attendance in schools or other facilities with facial recognition. The software is designed around the concept of having an IoT network of cameras in hallways providing facial feature data of those walking around the building to a centralized server and database. 

<img src="https://github.com/user-attachments/assets/f85f23b0-efab-405f-9bf7-7d334aaf4408" width="700px">
  
Algorithms were developed and tested in simulation to provide an accurate probabilistic estimate of attendance based on limited data, making up for the deficiencies in facial recognition algorithms by fusing data from multiple detections, estimated location of entities, and past habits or schedules. The system is designed to require minimal starting facial data or human interaction. 
The software includes three main C++ projects. The simulation stands in for the IoT network in a physical building and simulates students moving between classes in a building and devices detecting and extracting their facial features. The lambda stands in for a lambda function that would execute on the cloud with each facial detection reported from a device, and runs a first layer of processing, and the server executes processing on the database at period and day transitions. Locally, data is stored and communicated through a MySQL database. 

<img src="https://github.com/user-attachments/assets/8f3d1203-277f-4fbd-9b9e-d8dfbd86533b" width="500px">


Various algorithms were employed to convert the simulated detection data into accurate attendance data, including versions of Kalman filters, a particle filter, path graph learning, and flood fills. There is also a visualization app that allows for viewing the facial data set clusters by using a gradient decent algorithm to project the high-dimensional face space into 3d. 
The facial detection and recognition algorithms are from OpenCV and are here:  
https://github.com/ShiqiYu/libfacedetection  
https://github.com/zhongyy/SFace  
The dataset used was a subset of Labeled Faces in the Wild:  
https://www.kaggle.com/datasets/jessicali9530/lfw-dataset  


![sim](https://github.com/user-attachments/assets/64b6bdf8-fe1a-47ad-a276-2be9a6cc13b0)

![vis](https://github.com/user-attachments/assets/a33a38ba-80cb-4bb6-a91a-e9003bfca826)
