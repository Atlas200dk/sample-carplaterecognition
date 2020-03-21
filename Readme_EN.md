English|[中文](Readme.md)

# License Plate Recognition<a name="EN-US_TOPIC_0228461932"></a>

This application can run on the Atlas 200 DK to collect camera data in real time and predict vehicle license plate information in the video. The precision is low due to the model. To improve the precision, you can train a model to replace it.

The applications in the current version branch adapt to  [DDK&RunTime](https://ascend.huawei.com/resources) **1.32.0.0 and later**.

## Prerequisites<a name="section137245294533"></a>

Before deploying this sample, ensure that:

-   Mind Studio  has been installed.
-   The Atlas 200 DK developer board has been connected to  Mind Studio, the cross compiler has been installed, the SD card has been prepared, and basic information has been configured.

## Deployment<a name="section412811285117"></a>

You can use either of the following methods:

1.  Quick deployment: visit  [https://gitee.com/Atlas200DK/faster-deploy](https://gitee.com/Atlas200DK/faster-deploy).

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   The quick deployment script can be used to deploy multiple samples rapidly. Select  **carplaterecognition**.  
    >-   The quick deployment script automatically completes code download, model conversion, and environment variable configuration. To learn about the detailed deployment process, select the common deployment mode. Go to  **[2. Common deployment](#li3208251440)**.  

2.  <a name="li3208251440"></a>Common deployment: visit  [https://gitee.com/Atlas200DK/sample-READEME/tree/master/sample-carplaterecognition](https://gitee.com/Atlas200DK/sample-READEME/tree/master/sample-carplaterecognition).

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   In this deployment mode, you need to manually download code, convert models, and configure environment variables. After that, you will have a better understanding of the process.  


## Build<a name="section1759513564117"></a>

1.  Open the project.

    Go to the directory that stores the decompressed installation package as the Mind Studio installation user in CLI mode, for example,  **$HOME/MindStudio-ubuntu/bin**. Run the following command to start Mind Studio:

    **./MindStudio.sh**

    Open the  **sample-carplaterecognition**  project, as shown in  [Figure 1](#en-us_topic_0218873008_fig721144422212).

    **Figure  1**  Opening the sample-carplaterecognition project<a name="en-us_topic_0218873008_fig721144422212"></a>  
    

    ![](figures/en-us_image_0218876987.png)

2.  Configure project information in the  **src/param\_configure.conf**  file.

    **Figure  2**  Configuration file path<a name="en-us_topic_0218873008_fig1557065718252"></a>  
    

    ![](figures/en-us_image_0218877806.png)

    Content of the configuration file:

    ```
    remote_host=
    data_source=
    presenter_view_app_name=
    ```

    -   **remote\_host**: IP address of the Atlas 200 DK developer board
    -   **data\_source**: camera channel. The value can be  **Channel-1**  or  **Channel-2**. For details, see "Viewing the Channel to Which a Camera Belongs" in  [Atlas 200 DK User Guide](https://ascend.huawei.com/documentation).
    -   **presenter\_view\_app\_name**: value of  **View Name**  on the  **Presenter Server**  page, which must be unique. The value consists of at least one character and supports only uppercase letters, lowercase letters, digits, and underscores \(\_\).

    Configuration example:

    ```
    remote_host=192.168.1.2
    data_source=Channel-1
    presenter_view_app_name=video
    ```

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   All the three parameters must be set. Otherwise, the build fails.  
    >-   Do not use double quotation marks \(""\) during parameter settings.  

3.  Run the  **deploy.sh**  script to adjust configuration parameters and download and compile the third-party library. Open the  **Terminal**  window of Mind Studio. By default, the home directory of the code is used. Run the  **deploy.sh**  script in the background to deploy the environment, as shown in  [Figure 3](#en-us_topic_0218873008_fig19393122943511).

    **Figure  3**  Running the deploy.sh script<a name="en-us_topic_0218873008_fig19393122943511"></a>  
    ![](figures/running-the-deploy-sh-script-22.png "running-the-deploy-sh-script-22")

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   During the first deployment, if no third-party library is used, the system automatically downloads and builds the third-party library, which may take a long time. The third-party library can be directly used for the subsequent build.  
    >-   During deployment, select the IP address of the host that communicates with the developer board. Generally, the IP address is the IP address configured for the virtual NIC. If the IP address is in the same network segment as the IP address of the developer board, it is automatically selected for deployment. If they are not in the same network segment, you need to manually type the IP address of the host that communicates with the Atlas DK to complete the deployment.  

4.  Start building. Open Mind Studio and choose  **Build \> Build \> Build-Configuration**  from the main menu. The  **build**  and  **run**  folders are generated in the directory, as shown in  [Figure 4](#en-us_topic_0218873008_fig13819202814301).

    **Figure  4**  Build and file generation<a name="en-us_topic_0218873008_fig13819202814301"></a>  
    

    ![](figures/977f70e5b743c8ab6267c7d9b93194c.png)

    >![](public_sys-resources/icon-notice.gif) **NOTICE:**   
    >When you build a project for the first time,  **Build \> Build**  is unavailable. You need to choose  **Build \> Edit Build Configuration**  to set parameters before the build.  

5.  Start Presenter Server.

    Open the  **Terminal**  window of Mind Studio. Under the code storage path, run the following command to start the Presenter Server program of the license plate recognition application on the server, as shown in  [Figure 5](#en-us_topic_0218873008_fig102142024389).

    **bash run\_present\_server.sh**

    **Figure  5**  Starting Presenter Server<a name="en-us_topic_0218873008_fig102142024389"></a>  
    

    ![](figures/3510ef898c672612c4ac5fa9397c708.png)

    -   When the message  **Please choose one to show the presenter in browser\(default: 127.0.0.1\):**  is displayed, type the IP address \(usually IP address for accessing Mind Studio\) used for accessing the Presenter Server service in the browser.

        Select the IP address used by the browser to access the Presenter Server service in  **Current environment valid ip list**  and type the path for storing video analysis data, as shown in  [Figure 6](#en-us_topic_0218873008_fig73590910118).

        **Figure  6**  Project deployment<a name="en-us_topic_0218873008_fig73590910118"></a>  
        

        ![](figures/827447f108867314d18ff4e4b22d421.png)

    [Figure 7](#en-us_topic_0218873008_fig19953175965417)  shows that the Presenter Server service has been started successfully.

    **Figure  7**  Starting the Presenter Server process<a name="en-us_topic_0218873008_fig19953175965417"></a>  
    

    ![](figures/de3f85cb841628787ec4718cee44e8e.png)

    Use the URL shown in the preceding figure to log in to Presenter Server \(only Google Chrome is supported\). The IP address is that typed in  [Project deployment](#en-us_topic_0218873008_fig73590910118)  and the default port number is  **7007**. The following figure indicates that Presenter Server has been started successfully.

    **Figure  8**  Home page<a name="en-us_topic_0218873008_fig64391558352"></a>  
    ![](figures/home-page-23.png "home-page-23")

    The following figure shows the IP address used by Presenter Server and  Mind Studio  to communicate with the Atlas 200 DK.

    **Figure  9**  IP address example<a name="en-us_topic_0218873008_fig195318596543"></a>  
    ![](figures/ip-address-example-24.png "ip-address-example-24")

    -   The IP address of the Atlas 200 DK developer board is  **192.168.1.2**  \(connected in USB mode\).
    -   The IP address used by Presenter Server to communicate with the Atlas 200 DK is in the same network segment as the IP address of the Atlas 200 DK on the UI Host server. For example:  **192.168.1.223**.
    -   The following describes how to access the IP address \(such as  **10.10.0.1**\) of Presenter Server using a browser. Because Presenter Server and  Mind Studio  are deployed on the same server, you can access  Mind Studio  through the browser using the same IP address. 


## Run<a name="section6245151616426"></a>

1.  Run the license plate recognition application.

    On the toolbar of Mind Studio, click  **Run**  and choose  **Run \> Run 'sample-carplaterecognition'**. As shown in  [Figure 10](#en-us_topic_0218873008_fig12953163061713), the executable application is running on the developer board.

    **Figure  10**  Application running<a name="en-us_topic_0218873008_fig12953163061713"></a>  
    

    ![](figures/7c9dd1a0a188f9dc2f5f25e4023f965.png)

2.  Use the URL displayed upon the start of the Presenter Server service to log in to Presenter Server. For details, see  [Start Presenter Server](en-us_topic_0218873008.md#li499911453439).

    Wait for Presenter Agent to transmit data to the server. Click  **Refresh**. When there is data, the icon in the  **Status**  column for the corresponding channel changes to green, as shown in  [Figure 11](#en-us_topic_0218873008_fig113691556202312).

    **Figure  11**  Presenter Server page<a name="en-us_topic_0218873008_fig113691556202312"></a>  
    ![](figures/presenter-server-page-25.png "presenter-server-page-25")

    >![](public_sys-resources/icon-note.gif) **NOTE:**   
    >-   For the license plate recognition application, Presenter Server supports a maximum of 10 channels at the same time \(each  _presenter\_view\_app\_name_  parameter corresponds to a channel\).  
    >-   Due to hardware limitations, each channel supports a maximum frame rate of 20 fps. A lower frame rate is automatically used when the network bandwidth is low.  

3.  Click the link \(such as  **video**  in the preceding figure\) in the  **View Name**  column to view the result. The information of the detected license plate is displayed.

## Follow-up Operations<a name="section1092612277429"></a>

-   Stopping the license plate recognition application

    The license plate recognition application is running continually after being executed. To stop it, perform the following operation:

    Click the stop button shown in  [Figure 12](#en-us_topic_0218873008_fig14326454172518).

    **Figure  12**  Stopping the license plate recognition application<a name="en-us_topic_0218873008_fig14326454172518"></a>  
    

    ![](figures/9381c4c3003c7f2cf5479b0464b177f.png)

    [Figure 13](#en-us_topic_0218873008_fig2182182518112)  shows that the license plate recognition application has been stopped.

    **Figure  13**  Stopped license plate recognition application<a name="en-us_topic_0218873008_fig2182182518112"></a>  
    

    ![](figures/186ec128ed22c2f0edfabf73aae63dc.png)

-   Stopping the Presenter Server service

    The Presenter Server service is always in the running state after being started. To stop the Presenter Server service of the license plate recognition application, perform the following operations:

    On the server with  Mind Studio  installed, run the following command as the  Mind Studio  installation user to check the process of the Presenter Server service corresponding to the license plate recognition application:

    **ps -ef | grep presenter | grep carplate\_recognition**

    ```
    ascend@ascend-HP-ProDesk-600-G4-PCI-MT:~/sample-carplaterecognition$ ps -ef | grep presenter | grep carplate_recognition
    ascend    7701  1615  0 14:21 pts/8    00:00:00 python3 presenterserver/presenter_server.py --app carplate_recognition
    ```

    In the preceding information,  _7701_  indicates the process ID of the Presenter Server service corresponding to the license plate recognition application.

    To stop the service, run the following command:

    **kill -9** _7701_


