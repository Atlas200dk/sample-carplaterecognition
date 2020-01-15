# 车牌识别<a name="ZH-CN_TOPIC_0218676658"></a>

开发者可以将本application部署至Atlas 200DK上实现对摄像头数据的实时采集、并对视频中的车辆车牌信息进行预测的功能。

当前分支中的应用适配**1.1.1.0**与**1.3.0.0**版本的[DDK&RunTime](https://ascend.huawei.com/resources)。

## 前提条件<a name="section137245294533"></a>

部署此Sample前，需要准备好以下环境：

-   已完成Mind Studio的安装。
-   已完成Atlas 200 DK开发者板与Mind Studio的连接，交叉编译器的安装，SD卡的制作及基本信息的配置等。

## 软件准备<a name="section8534138124114"></a>

运行此Sample前，需要按照此章节进行相关的环境配置并获取源码包、准备模型文件。

1.  以Mind Studio安装用户登录Mind Studio所在Ubuntu服务器，并设置环境变量。

    **vim \~/.bashrc**

    执行如下命令在最后一行添加DDK\_HOME及LD\_LIBRARY\_PATH的环境变量。

    **export DDK\_HOME=/home/XXX/tools/che/ddk/ddk**

    **export LD\_LIBRARY\_PATH=$DDK\_HOME/uihost/lib**

    >![](public_sys-resources/icon-note.gif) **说明：**   
    >-   XXX为Mind Studio安装用户，/home/XXX/tools为DDK默认安装路径。  
    >-   如果此环境变量已经添加，则此步骤可跳过。  

    输入:wq!保存退出。

    执行如下命令使环境变量生效。

    **source \~/.bashrc**

2.  获取源码包。

    将[https://gitee.com/Atlas200DK/sample-carplaterecognition](https://gitee.com/Atlas200DK/sample-carplaterecognition)仓中的代码以Mind Studio安装用户下载至Mind Studio所在Ubuntu服务器的任意目录，例如代码存放路径为：_/home/ascend/sample-carplaterecognition_。

3.  <a name="li8221184418455"></a>获取此应用中所需要的原始网络模型。

    参考[表1](#table117203103464)获取此应用中所用到的原始网络模型及其对应的权重文件，并将其存放到Mind Studio所在Ubuntu服务器的任意目录，例如$HOME/ascend/models/carplaterecognition。

    **表 1**  车牌识别应用中使用的模型

    <a name="table117203103464"></a>
    <table><thead align="left"><tr id="row4859191074617"><th class="cellrowborder" valign="top" width="17.32173217321732%" id="mcps1.2.4.1.1"><p id="p18859111074613"><a name="p18859111074613"></a><a name="p18859111074613"></a>模型名称</p>
    </th>
    <th class="cellrowborder" valign="top" width="9.68096809680968%" id="mcps1.2.4.1.2"><p id="p17859171013469"><a name="p17859171013469"></a><a name="p17859171013469"></a>模型说明</p>
    </th>
    <th class="cellrowborder" valign="top" width="72.997299729973%" id="mcps1.2.4.1.3"><p id="p1385991094614"><a name="p1385991094614"></a><a name="p1385991094614"></a>模型下载路径</p>
    </th>
    </tr>
    </thead>
    <tbody><tr id="row1985913103461"><td class="cellrowborder" valign="top" width="17.32173217321732%" headers="mcps1.2.4.1.1 "><p id="p14859151016464"><a name="p14859151016464"></a><a name="p14859151016464"></a>car_plate_detection</p>
    </td>
    <td class="cellrowborder" valign="top" width="9.68096809680968%" headers="mcps1.2.4.1.2 "><p id="p108593100461"><a name="p108593100461"></a><a name="p108593100461"></a>车牌检测网络模型。</p>
    <p id="p1785921024614"><a name="p1785921024614"></a><a name="p1785921024614"></a>基于Caffe的Mobilenet-SSD模型。</p>
    </td>
    <td class="cellrowborder" valign="top" width="72.997299729973%" headers="mcps1.2.4.1.3 "><p id="p158596106460"><a name="p158596106460"></a><a name="p158596106460"></a>请参考<a href="https://gitee.com/HuaweiAscend/models/tree/master/computer_vision/object_detect/car_plate_detectio" target="_blank" rel="noopener noreferrer">https://gitee.com/HuaweiAscend/models/tree/master/computer_vision/object_detect/car_plate_detectio</a>目录中README.md下载原始网络模型文件及其对应的权重文件。</p>
    </td>
    </tr>
    <tr id="row08596101464"><td class="cellrowborder" valign="top" width="17.32173217321732%" headers="mcps1.2.4.1.1 "><p id="p178591510164619"><a name="p178591510164619"></a><a name="p178591510164619"></a>car_plate_recognition</p>
    </td>
    <td class="cellrowborder" valign="top" width="9.68096809680968%" headers="mcps1.2.4.1.2 "><p id="p1485911105469"><a name="p1485911105469"></a><a name="p1485911105469"></a>车牌号码识别网络模型。</p>
    <p id="p17859191018468"><a name="p17859191018468"></a><a name="p17859191018468"></a>基于Caffe的CNN模型。</p>
    </td>
    <td class="cellrowborder" valign="top" width="72.997299729973%" headers="mcps1.2.4.1.3 "><p id="p7859181094619"><a name="p7859181094619"></a><a name="p7859181094619"></a>请参考<a href="https://gitee.com/HuaweiAscend/models/tree/master/computer_vision/classification/car_plate_recognition" target="_blank" rel="noopener noreferrer">https://gitee.com/HuaweiAscend/models/tree/master/computer_vision/classification/car_plate_recognition</a>目录中README.md下载原始网络模型文件及其对应的权重文件。</p>
    </td>
    </tr>
    <tr id="row88591310124617"><td class="cellrowborder" valign="top" width="17.32173217321732%" headers="mcps1.2.4.1.1 "><p id="p685911013465"><a name="p685911013465"></a><a name="p685911013465"></a>vgg_ssd</p>
    </td>
    <td class="cellrowborder" valign="top" width="9.68096809680968%" headers="mcps1.2.4.1.2 "><p id="p1786011016461"><a name="p1786011016461"></a><a name="p1786011016461"></a>目标检测网络模型。</p>
    <p id="p086018109465"><a name="p086018109465"></a><a name="p086018109465"></a>基于Caffe的SSD512模型。</p>
    </td>
    <td class="cellrowborder" valign="top" width="72.997299729973%" headers="mcps1.2.4.1.3 "><p id="p1186071044613"><a name="p1186071044613"></a><a name="p1186071044613"></a>请参考<a href="https://gitee.com/HuaweiAscend/models/tree/master/computer_vision/object_detect/vgg_ssd" target="_blank" rel="noopener noreferrer">https://gitee.com/HuaweiAscend/models/tree/master/computer_vision/object_detect/vgg_ssd</a>目录中README.md下载原始网络模型文件及其对应的权重文件。</p>
    </td>
    </tr>
    </tbody>
    </table>

4.  将原始网络模型转换为适配昇腾AI处理器的模型，模型转换有Mind Studio工具转换和命令行转换两种方式。
    -   通过Mindstudio工具进行模型转换。
        1.  在Mind Studio操作界面的顶部菜单栏中选择“Tool \> Convert Model”，进入模型转换界面。
        2.  在弹出的**Convert Model**操作界面中，Model File与Weight File分别选择[步骤3](#li8221184418455)中下载的模型文件和权重文件。
            -   Model Name填写为[表1](#table117203103464)对应的**模型名称**。
            -   其他参数保持默认值。

        3.  单击**OK**开始转换模型。

            car\_plate\_detection、vgg\_ssd模型在转换的时候，会有报错，错误信息如下图所示。

            **图 1**  模型转换错误<a name="fig1842765585311"></a>  
            ![](figures/模型转换错误.jpg "模型转换错误")

            此时在DetectionOutput层的Suggestion中选择SSDDetectionOutput，并点击Retry。

            模型转换成功后，后缀为.om的离线模型存放地址为$HOME/tools/che/model-zoo/my-model/xxx。

    -   命令行模式下进行模型转换。
        1.  以Mind Studio安装用户进入存放原始模型的文件夹。

            **cd $HOME/ascend/models/carplaterecognition**

        2.  使用omg工具执行以下命令进行模型转换。

            ```
            ${DDK_HOME}/uihost/bin/omg --output="./XXX" --model="./XXX.prototxt" --framework=0 --ddk_version=${tools_version} --weight="./XXX.caffemodel" --input_shape=`head -1 ../../sample-carplaterecognition/script/shape_XXX` --insert_op_conf=../../sample-carplaterecognition/script/aipp_XXX.cfg --op_name_map=../../sample-carplaterecognition/script/reassign_operators
            ```

            >![](public_sys-resources/icon-note.gif) **说明：**   
            >-   input\_shape、insert\_op\_conf、op\_name\_map所需要的文件都在源码所在路径下的“sample-videoanalysiscar/script”目录下。所以填写这些参数所需的路径时，请根据您自己下载的源码中的文件路径填写对应参数路径。  
            >-   **XXX**为[表 车牌识别应用中使用的模型](#table117203103464)中的模型名称，转换时请替换填入需要转换模型的模型名称。其中car\_plate\_recognition模型转换时不需要op\_name\_map参数，如果没有删除不需要的参数，转换模型时会有报错。  
            >-   每个参数的具体意义可以在以下文档中了解[https://ascend.huawei.com/doc/Atlas200DK/1.3.0.0/zh/zh-cn\_topic\_0165968579.html](https://ascend.huawei.com/doc/Atlas200DK/1.3.0.0/zh/zh-cn_topic_0165968579.html)  


5.  将转换好的模型文件（.om文件）上传到“sample-carplaterecognition/script”目录下。

## 部署<a name="section1759513564117"></a>

1.  以Mind Studio安装用户进入车牌识别应用代码所在根目录，如:**/home/ascend/sample-carplaterecognition**。
2.  <a name="li08019112542"></a>执行部署脚本，进行工程环境准备，包括ascenddk公共库的编译与部署、Presenter Server服务器的配置等操作，其中Presenter Server用于接收Application发送过来的数据并通过浏览器进行结果展示。

    **bash deploy.sh** _host\_ip_ _model\_mode_

    -   _host\_ip_：对于Atlas 200 DK开发者板，即为开发者板的IP地址。对于AI加速云服务器，即为Host侧的IP地址。

    -   model\_mode代表模型文件的部署方式，默认为internet。
        -   **local**：若Mind Studio所在Ubuntu系统未连接网络，请使用local模式，执行此命令前，需要参考[依赖代码库下载](#section13807155164319)将依赖代码库的下载到“/sample-carplaterecognition/script“目录下。
        -   **internet**：若Mind Studio所在Ubuntu系统已连接网络，请使用internet模式，在线下载依赖代码库的下载。

    命令示例：

    **bash deploy.sh 192.168.1.2 internet**

    -   当提示“Please choose one to show the presenter in browser\(default: 127.0.0.1\):“时，请输入在浏览器中访问Presenter Server服务所使用的IP地址（一般为访问Mind Studio的IP地址）。

    如[图2](#fig184321447181017)所示，请在“Current environment valid ip list“中选择通过浏览器访问Presenter Server服务使用的IP地址。

    **图 2**  工程部署示意图<a name="fig184321447181017"></a>  
    

    ![](figures/zh-cn_image_0218722355.png)

3.  <a name="li499911453439"></a>启动Presenter Server。

    执行如下命令在后台启动车牌识别应用的Presenter Server主程序。

    **python3 presenterserver/presenter\_server.py --app carplate\_recognition &**

    >![](public_sys-resources/icon-note.gif) **说明：**   
    >“presenter\_server.py“在当前目录的“presenterserver“目录下，可以在此目录下执行**python3 presenter\_server.py -h**或者**python3 presenter\_server.py --help**查看“presenter\_server.py“的使用方法。  

    如[图3](#fig69531305324)所示，表示presenter\_server的服务启动成功。

    **图 3**  Presenter Server进程启动<a name="fig69531305324"></a>  
    ![](figures/Presenter-Server进程启动.png "Presenter-Server进程启动")

    使用上图提示的URL登录Presenter Server，仅支持Chrome浏览器，IP地址为[步骤2](#li08019112542)中输入的IP地址，端口号默为7007，如下图所示，表示Presenter Server启动成功。

    **图 4**  主页显示<a name="fig64391558352"></a>  
    ![](figures/主页显示.png "主页显示")

    Presenter Server、Mind Studio与Atlas 200 DK之间通信使用的IP地址示例如下图所示：

    **图 5**  IP地址示例<a name="fig1881532172010"></a>  
    ![](figures/IP地址示例.png "IP地址示例")

    -   Atlas 200 DK开发者板使用的IP地址为192.168.1.2（USB方式连接）。
    -   Presenter Server与Atlas 200 DK通信的IP地址为UI Host服务器中与Atlas 200 DK在同一网段的IP地址，例如：192.168.1.223。
    -   通过浏览器访问Presenter Server的IP地址本示例为：10.10.0.1，由于Presenter Server与Mind Studio部署在同一服务器，此IP地址也为通过浏览器访问Mind Studio的IP。


## 运行<a name="section6245151616426"></a>

1.  运行车牌识别应用程序。

    在“/home/ascend/sample-carplaterecognition“目录下执行如下命令运行车牌识别应用程序。

    **bash run\_carplaterecognitionapp.sh** _host\_ip_ _presenter\_view\_app\_name  camera\_channel\_name_   &

    -   _host\_ip_：对于Atlas 200 DK开发者板，即为开发者板的IP地址。
    -   _presenter\_view\_app\_name_：用户自定义的在PresenterServer界面展示的View Name，此View Name需要在Presenter Server展示界面唯一，只能为大小写字母、数字、“/”的组合，位数至少1位。
    -   _camera\_channel\_name_：摄像头所属Channel，取值为“Channel-1“或者“Channel-2“，查询摄像头所属Channel的方法请参考[Atlas 200 DK使用指南](https://ascend.huawei.com/documentation)中的“如何查看摄像头所属Channel”。

    命令示例：

    **bash run\_carplaterecognitionapp.sh 192.168.1.2 video Channel-1 &**

2.  使用启动Presenter Server服务时提示的URL登录 Presenter Server 网站，详细可参考[3](#li499911453439)。

    等待Presenter Agent传输数据给服务端，单击“Refresh“刷新，当有数据时相应的Channel 的Status变成绿色，如[图 Presenter Server界面](#fig113691556202312)所示。

    **图 6**  Presenter Server界面<a name="fig113691556202312"></a>  
    ![](figures/Presenter-Server界面.png "Presenter-Server界面")

    >![](public_sys-resources/icon-note.gif) **说明：**   
    >-   carplaterecognition的Presenter Server最多支持10路Channel同时显示，每个  _presenter\_view\_app\_name_  对应一路Channel。  
    >-   由于硬件的限制，每一路支持的最大帧率是20fps，受限于网络带宽的影响，帧率会自动适配为较低的帧率进行展示。  

3.  单击右侧对应的View Name链接，比如上图的“video”，查看结果，对于检测到的车辆车牌，会给出车牌号和置信度。

## 后续处理<a name="section1092612277429"></a>

-   **停止车牌识别应用**

    若要停止车牌识别应用程序，可执行如下操作。

    以Mind Studio安装用户在sample-carplaterecognition目录下执行如下命令：

    **bash stop\_carplaterecognitionapp.sh** _host\_ip_

    _host\_ip_：对于Atlas 200 DK开发者板，即为开发者板的IP地址。对于AI加速云服务器，即为Host的IP地址。。

    命令示例：

    **bash stop\_carplaterecognitionapp.sh 192.168.1.2**

-   **停止Presenter Server服务**

    Presenter Server服务启动后会一直处于运行状态，若想停止车牌识别应用对应的Presenter Server服务，可执行如下操作。

    以Mind Studio安装用户在Mind Studio所在服务器中执行如下命令查看车牌识别应用对应的Presenter Server服务的进程。

    **ps -ef | grep present | grep carplate\_recognition**

    ```
    ascend@ascend-HP-ProDesk-600-G4-PCI-MT:~/sample-videoanalysiscar$ ps -ef | grep presenter | grep carplate_recognition
    ascend 3655 20313 0 15:10 pts/24?? 00:00:00 python3 presenterserver/presenter_server.py --app carplate_recognition
    ```

    如上所示  _3655_  即为车牌识别应用对应的Presenter Server服务的进程ID。

    若想停止此服务，执行如下命令：

    **kill -9** _3655_


## 依赖代码库下载<a name="section13807155164319"></a>

将依赖的软件库下载到“sample-carplaterecognition/script“目录下。

**表 2**  依赖代码库下载

<a name="table11339161214583"></a>
<table><thead align="left"><tr id="row43391112135817"><th class="cellrowborder" valign="top" width="33.33333333333333%" id="mcps1.2.4.1.1"><p id="p13395121583"><a name="p13395121583"></a><a name="p13395121583"></a>模块名称</p>
</th>
<th class="cellrowborder" valign="top" width="33.33333333333333%" id="mcps1.2.4.1.2"><p id="p433981216581"><a name="p433981216581"></a><a name="p433981216581"></a>模块描述</p>
</th>
<th class="cellrowborder" valign="top" width="33.33333333333333%" id="mcps1.2.4.1.3"><p id="p4339181265819"><a name="p4339181265819"></a><a name="p4339181265819"></a>下载地址</p>
</th>
</tr>
</thead>
<tbody><tr id="row1034014127588"><td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.1 "><p id="p13340312145811"><a name="p13340312145811"></a><a name="p13340312145811"></a>EZDVPP</p>
</td>
<td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.2 "><p id="p1334011245818"><a name="p1334011245818"></a><a name="p1334011245818"></a>对DVPP接口进行了封装，提供对图片/视频的处理能力。</p>
</td>
<td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.3 "><p id="p4340112155815"><a name="p4340112155815"></a><a name="p4340112155815"></a><a href="https://gitee.com/Atlas200DK/sdk-ezdvpp" target="_blank" rel="noopener noreferrer">https://gitee.com/Atlas200DK/sdk-ezdvpp</a></p>
<p id="p18340012195812"><a name="p18340012195812"></a><a name="p18340012195812"></a>下载后请保持文件夹名称为ezdvpp。</p>
</td>
</tr>
<tr id="row14340191245813"><td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.1 "><p id="p19340812135812"><a name="p19340812135812"></a><a name="p19340812135812"></a>Presenter Agent</p>
</td>
<td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.2 "><p id="p734081285817"><a name="p734081285817"></a><a name="p734081285817"></a>与Presenter Server进行交互的API接口。</p>
</td>
<td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.3 "><p id="p14340912175819"><a name="p14340912175819"></a><a name="p14340912175819"></a><a href="https://gitee.com/Atlas200DK/sdk-presenter/tree/master" target="_blank" rel="noopener noreferrer">https://gitee.com/Atlas200DK/sdk-presenter/tree/master</a></p>
<p id="p1334013129584"><a name="p1334013129584"></a><a name="p1334013129584"></a>请获取此路径下的presenteragent文件夹，下载后请保持文件夹名称为presenteragent。</p>
</td>
</tr>
<tr id="row7341151285813"><td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.1 "><p id="p434111212585"><a name="p434111212585"></a><a name="p434111212585"></a>tornado (5.1.0)</p>
<p id="p1534151245813"><a name="p1534151245813"></a><a name="p1534151245813"></a>protobuf (3.5.1)</p>
<p id="p1834114127589"><a name="p1834114127589"></a><a name="p1834114127589"></a>numpy (1.14.2)</p>
</td>
<td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.2 "><p id="p1334111217584"><a name="p1334111217584"></a><a name="p1334111217584"></a>Presenter Server依赖的Python库</p>
</td>
<td class="cellrowborder" valign="top" width="33.33333333333333%" headers="mcps1.2.4.1.3 "><p id="p83411512205814"><a name="p83411512205814"></a><a name="p83411512205814"></a>可以在python官网<a href="https://pypi.org/" target="_blank" rel="noopener noreferrer">https://pypi.org/</a>上搜索相关包进行安装。</p>
<p id="p534111129588"><a name="p534111129588"></a><a name="p534111129588"></a>若使用pip3 install命令在线下载，可以使用如下命令指定相关版本进行下载，例如</p>
<p id="p73418128585"><a name="p73418128585"></a><a name="p73418128585"></a>pip3 install tornado==5.1.0  -i  <em id="i18341131295818"><a name="i18341131295818"></a><a name="i18341131295818"></a>指定库的安装源</em>  --trusted-host  <em id="i93411612205812"><a name="i93411612205812"></a><a name="i93411612205812"></a>安装源的主机名</em></p>
</td>
</tr>
</tbody>
</table>

