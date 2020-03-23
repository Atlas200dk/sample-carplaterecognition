中文|[English](Readme_EN.md)

# 车牌识别<a name="ZH-CN_TOPIC_0228461932"></a>

开发者可以将本application部署至Atlas 200DK上实现对摄像头数据的实时采集、并对视频中的车辆车牌信息进行预测的功能。受模型影响，精准度较低，如果需要提高精度，可以自己训练模型替换。

当前分支中的应用适配**1.32.0.0及以上**版本的[DDK&RunTime](https://ascend.huawei.com/resources)。

## 前提条件<a name="section137245294533"></a>

部署此Sample前，需要准备好以下环境：

-   已完成Mind Studio的安装。
-   已完成Atlas 200 DK开发者板与Mind Studio的连接，交叉编译器的安装，SD卡的制作及基本信息的配置等。

## 部署<a name="section412811285117"></a>

可以选择如下快速部署或者常规方法部署，二选一即可：

1.  快速部署，请参考：  [https://gitee.com/Atlas200DK/faster-deploy](https://gitee.com/Atlas200DK/faster-deploy)  。

    >![](public_sys-resources/icon-note.gif) **说明：**   
    >-   该快速部署脚本可以快速部署多个案例，请选择carplaterecognition案例部署即可。  
    >-   该快速部署脚本自动完成了代码下载、模型转换、环境变量配置等流程，如果需要了解详细的部署过程请选择常规部署方式。转: **[2. 常规部署](#li3208251440)**  

2.  <a name="li3208251440"></a>常规部署，请参考：  [https://gitee.com/Atlas200DK/sample-README/tree/master/sample-carplaterecognition](https://gitee.com/Atlas200DK/sample-README/tree/master/sample-carplaterecognition)  。

    >![](public_sys-resources/icon-note.gif) **说明：**   
    >-   该部署方式，需要手动完成代码下载、模型转换、环境变量配置等过程。完成后，会对其中的过程更加了解。  


## 编译<a name="section1759513564117"></a>

1.  打开对应的工程。

    以Mind Studio安装用户在命令行中进入安装包解压后的“MindStudio-ubuntu/bin”目录，如：$HOME/MindStudio-ubuntu/bin。执行如下命令启动Mind Studio

    **./MindStudio.sh**

    启动成功后，打开**sample-carplaterecognition**工程，如[图 打开sample-carplaterecognition工程](#zh-cn_topic_0218873008_fig721144422212)所示。

    **图 1**  打开sample-carplaterecognition工程<a name="zh-cn_topic_0218873008_fig721144422212"></a>  
    

    ![](figures/zh-cn_image_0218876987.png)

2.  在src/param\_configure.conf文件中配置相关工程信息。

    **图 2**  配置文件路径<a name="zh-cn_topic_0218873008_fig1557065718252"></a>  
    

    ![](figures/zh-cn_image_0218877806.png)

    该配置文件内容如下：

    ```
    remote_host=
    data_source=
    presenter_view_app_name=
    ```

    -   remote\_host：配置为Atlas 200 DK开发者板的IP地址。
    -   data\_source : 配置摄像头所属Channel，取值为Channel-1或者Channel-2，查询摄像头所属Channel的方法请参考[Atlas 200 DK用户手册](https://ascend.huawei.com/doc/Atlas200DK/)中的“如何查看摄像头所属Channel”。
    -   presenter\_view\_app\_name : 用户自定义的在PresenterServer界面展示的View Name，此View Name需要在Presenter Server展示界面唯一，只能为大小写字母、数字、“/”的组合，位数至少1位。

    配置示例：

    ```
    remote_host=192.168.1.2
    data_source=Channel-1
    presenter_view_app_name=video
    ```

    >![](public_sys-resources/icon-note.gif) **说明：**   
    >-   三个参数必须全部填写，否则无法通过编译。  
    >-   注意参数填写时不需要使用“”符号。  

3.  执行deploy脚本， 进行配置参数调整及第三方库下载编译 打开Mind Studio工具的Terminal，此时默认在代码主目录下，执行如下命令在后台指执行deploy脚本，进行环境部署。如[图 执行deploy脚本](#zh-cn_topic_0218873008_fig19393122943511)所示。

    **图 3**  执行deploy脚本<a name="zh-cn_topic_0218873008_fig19393122943511"></a>  
    ![](figures/执行deploy脚本-24.png "执行deploy脚本-24")

    >![](public_sys-resources/icon-note.gif) **说明：**   
    >-   首次deploy时，没有部署第三方库时会自动下载并编译，耗时可能比较久，请耐心等待。后续再重新编译时，不会重复下载编译，部署如上图所示。  
    >-   deploy时，需要选择与开发板通信的主机侧ip，一般为虚拟网卡配置的ip。如果此ip和开发板ip属于同网段，则会自动选择并部署。如果非同网段，则需要手动输入与开发板通信的主机侧ip才能完成deploy。  

4.  开始编译，打开Mind Studio工具，在工具栏中点击**Build \> Build \> Build-Configuration**。如[图4](#zh-cn_topic_0218873008_fig13819202814301)所示，会在目录下生成build和run文件夹。

    **图 4**  编译操作及生成文件<a name="zh-cn_topic_0218873008_fig13819202814301"></a>  
    

    ![](figures/977f70e5b743c8ab6267c7d9b93194c.png)

    >![](public_sys-resources/icon-notice.gif) **须知：**   
    >首次编译工程时，**Build \> Build**为灰色不可点击状态。需要点击**Build \> Edit Build Configuration**，配置编译参数后再进行编译。  

5.  启动Presenter Server。<a name="zh-cn_topic_0218873008_fig12953163062222"></a> 

    打开Mindstudio工具的Terminal，在应用代码存放路径下，执行如下命令在后台启动carplate recognition应用的Presenter Server主程序。如[图 启动PresenterServer](#zh-cn_topic_0218873008_fig102142024389)所示。

    **bash run\_present\_server.sh**

    **图 5**  启动PresenterServer<a name="zh-cn_topic_0218873008_fig102142024389"></a>  
    

    ![](figures/3510ef898c672612c4ac5fa9397c708.png)

    -   当提示“Please choose one to show the presenter in browser\(default: 127.0.0.1\):“时，请输入在浏览器中访问Presenter Server服务所使用的IP地址（一般为访问Mind Studio的IP地址）。

        如[图6](#zh-cn_topic_0218873008_fig73590910118)所示，请在“Current environment valid ip list“中选择通过浏览器访问Presenter Server服务使用的IP地址，并输入存储视频解析数据的路径。

        **图 6**  工程部署示意图<a name="zh-cn_topic_0218873008_fig73590910118"></a>  
        

        ![](figures/827447f108867314d18ff4e4b22d421.png)

    如[图7](#zh-cn_topic_0218873008_fig19953175965417)所示，表示presenter\_server的服务启动成功。

    **图 7**  Presenter Server进程启动<a name="zh-cn_topic_0218873008_fig19953175965417"></a>  
    

    ![](figures/de3f85cb841628787ec4718cee44e8e.png)

    使用上图提示的URL登录Presenter Server，仅支持Chrome浏览器。IP地址为[图 工程部署示意图](#zh-cn_topic_0218873008_fig73590910118)操作时输入的IP地址，端口号默为7007，如下图所示，表示Presenter Server启动成功。

    **图 8**  主页显示<a name="zh-cn_topic_0218873008_fig64391558352"></a>  
    ![](figures/主页显示-25.png "主页显示-25")

    Presenter Server、Mind Studio与Atlas 200 DK之间通信使用的IP地址示例如下图所示：

    **图 9**  IP地址示例<a name="zh-cn_topic_0218873008_fig195318596543"></a>  
    ![](figures/IP地址示例-26.png "IP地址示例-26")

    -   Atlas 200 DK开发者板使用的IP地址为192.168.1.2（USB方式连接）。
    -   Presenter Server与Atlas 200 DK通信的IP地址为UI Host服务器中与Atlas 200 DK在同一网段的IP地址，例如：192.168.1.223。
    -   通过浏览器访问Presenter Server的IP地址本示例为：10.10.0.1，由于Presenter Server与Mind Studio部署在同一服务器，此IP地址也为通过浏览器访问Mind Studio的IP。


## 运行<a name="section6245151616426"></a>

1.  运行车牌识别应用程序

    在Mind Studio工具的工具栏中找到Run按钮，点击  **Run \> Run 'sample-carplaterecognition'**  ，如[图10](#zh-cn_topic_0218873008_fig12953163061713)所示，可执行程序已经在开发板执行。

    **图 10**  程序运行示意图<a name="zh-cn_topic_0218873008_fig12953163061713"></a>  
    

    ![](figures/7c9dd1a0a188f9dc2f5f25e4023f965.png)

2.  使用启动Presenter Server服务时提示的URL登录 Presenter Server 网站，详细可参考[启动Presenter Server](#zh-cn_topic_0218873008_fig12953163062222)。 

    等待Presenter Agent传输数据给服务端，单击“Refresh“刷新，当有数据时相应的Channel 的Status变成绿色，如[图 Presenter Server界面](#zh-cn_topic_0218873008_fig113691556202312)所示。

    **图 11**  Presenter Server界面<a name="zh-cn_topic_0218873008_fig113691556202312"></a>  
    ![](figures/Presenter-Server界面-27.png "Presenter-Server界面-27")

    >![](public_sys-resources/icon-note.gif) **说明：**   
    >-   Carplate recognition的Presenter Server最多支持10路Channel同时显示，每个  _presenter\_view\_app\_name_  对应一路Channel。  
    >-   由于硬件的限制，每一路支持的最大帧率是20fps，受限于网络带宽的影响，帧率会自动适配为较低的帧率进行展示。  

3.  单击右侧对应的View Name链接，比如上图的“video”，查看结果，对于检测到的车牌，会给出车牌号信息。

## 后续处理<a name="section1092612277429"></a>

-   **停止车牌识别应用**

    车牌识别应用执行后会处于持续运行状态，若要停止车牌识别应用程序，可执行如下操作。

    单击[图 停止车牌识别应用](#zh-cn_topic_0218873008_fig14326454172518)所示的停止按钮停止车牌识别应用程序。

    **图 12**  停止车牌识别应用<a name="zh-cn_topic_0218873008_fig14326454172518"></a>  
    

    ![](figures/9381c4c3003c7f2cf5479b0464b177f.png)

    如[图 车牌识别应用已停止](#zh-cn_topic_0218873008_fig2182182518112)所示应用程序已停止运行

    **图 13**  车牌识别应用已停止<a name="zh-cn_topic_0218873008_fig2182182518112"></a>  
    

    ![](figures/186ec128ed22c2f0edfabf73aae63dc.png)

-   **停止Presenter Server服务**

    Presenter Server服务启动后会一直处于运行状态，若想停止车牌识别应用对应的Presenter Server服务，可执行如下操作。

    以Mind Studio安装用户在Mind Studio所在服务器中的命令行中执行如下命令查看车牌识别应用对应的Presenter Server服务的进程。

    **ps -ef | grep presenter | grep carplate\_recognition**

    ```
    ascend@ascend-HP-ProDesk-600-G4-PCI-MT:~/sample-carplaterecognition$ ps -ef | grep presenter | grep carplate_recognition
    ascend    7701  1615  0 14:21 pts/8    00:00:00 python3 presenterserver/presenter_server.py --app carplate_recognition
    ```

    如上所示  _7701_  即为车牌识别应用对应的Presenter Server服务的进程ID。

    若想停止此服务，执行如下命令：

    **kill -9** _7701_


