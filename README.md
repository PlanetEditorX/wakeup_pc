# 利用小爱同学控制局域网电脑开关机

---

### 一、使用docker

---

#### （一）远程拉取（推荐）
##### 1. 拉取镜像

- x86/arm64
  ```bash
  docker pull yexundao/wakeup_pc:latest
  ```

  - 使用`Publish Docker image`手动构建或直接打上tags自动构建
  - x86和arm64都可以使用，默认推荐镜像。

- arm64
  ```bash
  docker pull yexundao/wakeup_pc:latest-arm
  ```

  - 使用`Publish Docker image by self-hosted runner`手动构建下才使用这种镜像。
  - 这种方式需要配置好自己的arm服务器的自建runner，连接仓库后arm版本在本地构建。
  - 自建runner安装方式参考 https://github.com/PlanetEditorX/wakeup_pc/settings/actions/runners/new 和 [在ARMbian系统上将自托管运行器注册为系统服务并使其在后台运行](attachment/在ARMbian系统上将自托管运行器注册为系统服务并使其在后台运行.md)
  - 不管哪种方式x86都可以使用，区别是使用`by self-hosted runner`方式将arm版本给区别出来，arm只能使用`latest-arm`镜像。

- 默认镜像latest为C编译好的，1.8.0即之后的版本都是C编译版本，需要python版本需要手动执行Publish Docker Image Python的Actions
- 镜像体积缩小到十几兆，完全符合日常轻度使用。![image-20250101171824717](attachment/image-20250101171824717.png)


##### 2. 创建配置文件
- 在任意指定位置创建`config.ini`文件，如：
  - x86：`/vol1/1000/docker/wakeup/config.ini`
  - arm64：`/root/soft/wakeup/config.ini`

- 按照配置文件说明进行相应的配置，可在仓库查找模板或直接保存 https://raw.githubusercontent.com/PlanetEditorX/wakeup_pc/refs/heads/main/c/config.ini
- 在本地调试时，可通过指令忽略配置文件，避免配置上传到仓库`git update-index --assume-unchanged ~/wakeup_pc/docker/config.ini`

##### 3. 运行镜像

- 带日志和配置命令启动

  - 在宿主机配置文件目录下提前建立日志文件，如：

    - x86：`touch /vol1/1000/docker/wakeup/log.txt`

    - arm64：`touch /root/soft/wakeup/log.txt`

    - 注意：如果带日志命令却没有创建日志文件，docker容器将会自动创建一个`log.txt`的目录，无法正常输出日志
    - 日志文件默认最大为100kb大小，超过该大小会在每日凌晨一点进行日志清理操作，在`clear_logs.sh`中可以根据需要自定义大小数值

  - 创建并启动容器

    - x86
      ```bash
      docker run -d \
        --name wakeup_pc \
        -v /vol1/1000/docker/wakeup/config.ini:/app/config.ini \
        -v /vol1/1000/docker/wakeup/log.txt:/app/log.txt \
        --restart always \
        --network host \
        yexundao/wakeup_pc:latest
      ```

    - arm64
      ```bash
      docker run -d \
        --name wakeup_pc \
        -v /root/soft/wakeup/config.ini:/app/config.ini \
        -v /root/soft/wakeup/log.txt:/app/log.txt \
        --restart always \
        --network host \
        yexundao/wakeup_pc:latest
      ```

      或自建runner版本

      ```bash
      docker run -d \
        --name wakeup_pc \
        -v /root/soft/wakeup/config.ini:/app/config.ini \
        -v /root/soft/wakeup/log.txt:/app/log.txt \
        --restart always \
        --network host \
        yexundao/wakeup_pc:latest-arm
      ```

- 仅配置命令
  ```bash
  docker run -d \
    --name wakeup_pc \
    -v /vol1/1000/docker/wakeup/config.ini:/app/config.ini \
    --restart always \
    --network host \
    yexundao/wakeup_pc:latest
  ```

- 需要先根据自己的具体信息配置好config.ini文件

- 将运行命令中`/vol1/1000/docker/wakeup/config.ini`修改为自己实际配置文件地址

- 注意，这里容器里的目录位置是/app/

- 如果需要查看输出日志，可进入容器排查
  ```bash
  docker exec -it wakeup_pc sh
  ```

  <div style="text-align: left;">
      <img src="attachment/image-20250101172542475.png" alt="description">
  </div>

- 如果是带日志的命令启动，可直接在docker的宿主机查看日志
  ```bash
  tail -f /vol1/1000/docker/wakeup/log.txt
  ```

  <div style="text-align: left;">
      <img src="attachment/image-20250101173848545.png" alt="description">
  </div>

- 如果是空日志文件，则第一行输出关机命令的参数，如`cmd_shutdown: sshpass -p 密码 ssh -A -g -o StrictHostKeyChecking=no 用户名@IP 'shutdown /s /t 10'`，可通过对比相关数据和config.ini的参数是否匹配，如果为空则说明参数读取异常，检查config.ini配置文件的内容和位置是否正确。

- 和巴法云建立TCP连接
  ```shell
  The IP address of bemfa.com is: 119.91.109.180
  Heartbeat sent
  recv: cmd=1&res=1
  ```

  会向巴法云的IP发送TCP请求，看返回的是否是`recv: cmd=1&res=1`，全是1才是订阅主题成功，其它情况检查是否参数异常。



---

#### （二）本地构建

##### 1.环境

- 由于是在Linux上进行的开发，所以wakeup.c文件的部分头文件会报错，如果是WIN进行编译修改可能需要转为Windows的替代头文件

##### 2. 创建 Dockerfile
- 创建一个Dockerfile来构建alpine 容器，并在其中设置脚本。
```Dockerfile
# 使用alpine作为基础镜像，并安装必要的编译工具和库
FROM alpine:3.18.6 as builder

# 安装musl-dev和libc-dev，它们包含了C标准库的头文件和静态库
RUN apk add --no-cache musl-dev libc-dev gcc

# 设置工作目录
WORKDIR /app

# 复制源代码和配置文件到工作目录
COPY wakeup.c config.ini /app/

# 静态编译C程序，确保使用静态链接
RUN gcc -static -o wakeup wakeup.c

# 使用alpine作为最终的基础镜像
FROM alpine:3.18.6

# 从构建阶段复制静态编译的可执行文件到最终镜像
COPY --from=builder /app/wakeup /app/
COPY start.sh config.ini crontab /app/
# 设置工作目录
WORKDIR /app
RUN apk add --no-cache sshpass openssh && \
    chmod +x ./start.sh

# 设置容器启动时执行的命令
CMD ["./start.sh"]
```
##### 3. 构建 Docker 镜像
- 使用以下命令构建 Docker 镜像，该命令将会创建一个名为wakeup_pc的本地镜像：
```bash
docker build -t wakeup_pc .
```
```bash
docker images
```
![image-20250101174959871](attachment/image-20250101174959871.png)

- 如果多次构建，会产生很多标签为`none`的镜像，使用命令`docker rmi $(docker images -f "dangling=true" -q)`删除无用镜像。

##### 4. 创建配置文件
- 从仓库 https://raw.githubusercontent.com/PlanetEditorX/wakeup_pc/refs/heads/main/c/config.ini 中获取并保存在Linux主机上
    ![](attachment/6bf0e7d8b2602710526d63e25efd9d385cd60c4f5a804008d3124d18ec03f8a0.png)
- config.ini详情见远程拉取和iStoreOS的配置文件相关说明
##### 5. 运行 Docker 容器
```bash
docker run -d --restart=unless-stopped --name wakeup_pc --network host wakeup_pc
```
###### 命令解释
- -d：参数，表示在后台运行容器。
- --restart=unless-stopped：参数，设置容器的重启策略。unless-stopped 意味着容器将自动重启除非它被明确停止（例如，通过 docker stop）或者 Docker 本身被停止。
- --name wakeup_pc：参数，为容器指定一个名称，这里是 wakeup_pc。
- --network host：参数，将容器的网络设置为 host 模式，这意味着容器将不会获得自己的网络接口，而是使用宿主机的网络接口，这样才能访问局域网的主机进行开机和关机操作。
##### 5. 进入容器进行数据查看和排查故障
```bash
docker exec -it wakeup_pc sh
```

---
#### （三）利用Actions构建docker镜像

##### 1. <span class='custom-title-span'>按照说明逐一修改配置文件</span>

- 主要为两个流程`Publish Docker Image C` 和 `Publish Docker Image Python`，`Publish Docker Image main`实际指向C，只是增加了tags触发。
  ![image-20250103150452629](attachment/image-20250103150452629.png)

- 选择哪种语言直接点击工作流后，点击Run workflow就可以创建并推送对应语言的镜像到Docker Hub，实际使用并无多大差别，直接拉取最新的`latest`镜像就可以，无需在意具体的版本

![image-20250103150936440](attachment/image-20250103150936440.png)

![image-20250110111640950](attachment/image-20250110111640950.png)

---

#### （四）修改配置文件

##### 1. <span class='custom-title-span'>按照说明逐一修改配置文件</span>
##### 2. 巴法云私钥/client_id
- ![](attachment/edbafcef17a510c7a8458197b5457e0db13bece2a8efa74a5a62f44d5aad4dca.png)
##### 3. 主题值/topic
- ![](attachment/ec00e02c5afe8156849b147c4dc67ef840b8883d6697d428f58d8cfafe26287e.png)
##### 4. 设备MAC地址/mac
（1）需要唤醒的设备输入：`ipconfig /all`，找到支持唤醒的网卡的物理地址，注意：如果显示的物理地址为XX-XX-XX-XX-XX-XX，需要将短横杠替换为冒号，XX:XX:XX:XX:XX:XX
![](attachment/b0ac334a7df49d4540ffb28c251eb3249de04ebf240f8f7b362bac89e737e053.png)

##### 5. 远程电脑IP地址/ip
（1）需要唤醒的设备输入：`ipconfig`，根据自己的网卡找到IP地址
 ![](attachment/ccbf0a8dbf57c64da091c6cb5d15499633906e866843e7bc5a910b25a457d3fa.png)

##### 6. 远程SSH用户账号/user
- 设置的用户名
##### 7. 远程SSH用户密码/password
- 设置的用户密码



---

### 二、使用home assistant

---

#### （一）安装home assistant

##### 1. 使用飞牛OS的应用中心安装

##### 2.使用docker自行安装

- https://www.home-assistant.io/installation/linux

#### （二）文件配置

##### 1. Windows开启ssh服务器

##### 2.通过页面或其它方式进入终端

- 安装sshpass：`apk add sshpass`

  - 不直接使用密钥免密登录的原因：密钥方式虽然较直接携带密码的ssh方式更加安全，但通用性不足。特别是要docker和home assistant都使用的情况下，docker每次重新创建不必手动添加密钥到主机上。而且本身是创建了一个专用的ssh账户来管理关机事项，对于局域网来说，安全已经足够，不至于担心密码泄露的问题，没必要。

- 终端修改`configuration.yaml`文件，或打开飞牛OS的`文件管理`-`应用文件`-`home-assistantan`-`config`-`configuration.yaml`，在文件末尾添加

  - ```yaml
    switch:
      - platform: wake_on_lan
        name: "PC1"                     		# 定义HA中实体的名称
        mac: "00:00:00:00:00:00"                # 主机MAC地址
        host: "192.168.3.X"                     # 主机IP地址
        broadcast_address: "255.255.255.255"    # 广播地址
        broadcast_port: 9               		# 指定wol端口
        turn_off:
          service: shell_command.shutdown
    shell_command:
      shutdown: "sshpass -p SSH密码 ssh -A -g -o StrictHostKeyChecking=no SSH用户名@192.168.3.X \"shutdown -s -t 10\""         # 运行的关机命令
    ```

- 来到`home assistant`的`开发者工具`，在`配置检查与重启`中点击`检查配置`，显示`配置不会阻止 Home Assistant 启动！`后点击`重新启动`，点击`重新启动Home Assistant`后等待重启

#### （三）配置页面

##### 1. 查看是否配置成功

- 在`设置`-`设置与服务`-`实体`中查看是否有自定义的实体名称比如`PC1`，是一个按钮形式，点击切换查看能否开关电脑。

##### 2.首页配置按钮

- 在`概览`中点击右上角的编辑（铅笔）按钮，点击`添加卡片`，切换到`按实体`页面，搜索`PC1`，将其添加到首页。

##### 3.添加到iPhone家庭

- 到`HomeKit Bridge`点击`添加条目`，`要包含的域`就选择`Switch`，点击`提交`
- 在`集成条目`中新增加的项的右侧，点击`配置`，`包含模式`选择`include`，要包含的域选择`Switch`，点击提交
- 实体搜索`PC1`，选择后提交
- `通知`会显示`HomeKit Pairing`的二维码，iPhone家庭扫描添加

##### 4.手机端设置

- 为了更好的语音唤醒，可以手动修改下配件的名字
- 长按添加的唤醒组件，点击右下角的齿轮，编辑`房间`和`名称`，比如这里可以为`卧室`和`电脑`
- 在`设置`的`控制中心`中添加`家庭`到`控制中心`



---

### 三、配置成功

---

##### 1. 网页控制

- 巴法手动发送`on`或`off`控制开关机（可远程）
- 在`Home Assistant`点击按钮开关切换开关机

##### 2.App控制

- 巴法客户端控制（可远程）
- `Home Assistant`手机客户端点击按钮开关切换开关机
- iPhone`家庭`中或`控制中心`中快捷控制开关机

##### 3.语音控制

- 小爱同学语音控制：`小爱同学，打开电脑`、`小爱同学，关闭电脑`
- Siri语音控制：`嗨,Siri，打开卧室电脑`、`嗨,Siri，关闭卧室电脑`



---

### 四、SSH服务器配置

---
#### （一）启用OpenSSH
1. 设置—系统—可选功能—添加功能—OpenSSH服务器
2. 不成功参考 https://learn.microsoft.com/en-us/windows-server/administration/openssh/openssh_install_firstuse?tabs=powershell 或网上查找其它解决办法
 ![](attachment/eaac19363e418ae7c52b5437ca8420638b6d0c20e73562bb7bc0c7ed85ebfcf8.png)

---
#### （二）创建用户
##### 1. <span class='custom-title-span'>可以直接当前的本地用户，但由于密码是明文传输，为了增加一点点安全性，创建一个用于ssh登录的账户。</span>
##### 2. 打开编辑本地用户和组：win+r，输入lusrmgr.msc
-  ![](attachment/fbaebcdf0446ed068154bd1fcfc1a432d60d2758b242dc701a5dad9929907b56.png)
##### 3. <span class='custom-title-span'>输入用户名和密码，此处的用户名和密码就是SSH的用户名和密码，添加到配置的user和password</span>
##### 4. <span class='custom-title-span'>取消用户下次登录时须更改密码（M）的勾选，勾选用户不能更改密码(S)和密码永不过期(W)</span>

---
#### （三）测试用户
> 1. 打开powershell
> 2. 输入ssh 用户名@IP
> 3. 初次会提示输入yes
> 4. 输入隐形密码
> 5. 前方由文件地址转为用户名@主机名表示成功

---
#### （四）隐藏用户
##### 1. <span class='custom-title-span'>避免多用户时会在登录页面显示不需要的用户，参考https://www.ithome.com/0/228/192.htm</span>
##### 2. 确认要隐藏账户全名
（1）在开始按钮单击右键，选择“计算机管理”

（2）进入系统工具→本地用户和组→用户，在中间找到目标用户账户名

（3）记录好账户全名（本地账户没有“全名”，记录好“名称”即可）

##### 3. 新建注册表特殊账户
（1）`win+r`输入`regedit`后回车进入注册表编辑器

（2）定位到`HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Winlogon`

（3）在`Winlogon`单击右键新建“项”，命名为`SpecialAccounts`

（4）在`SpecialAccounts`单击右键新建“项”，命名为`UserList`

（5）在`UserList`单击右键`新建DWORD（32位）值`（如上图），命名为第一步中记录的账户全名（或本地账户名称），数值数据保持默认的0，此时进入锁屏，隐藏用户不可见。

![](attachment/d8091cddf27cd78b38327d115f504c9df0e7e3eb4541e34fc8785fbc35c12258.png)

---

#### （五）权限限制（可选）
##### 1. <span class='custom-title-span'>适当的限制权限，避免别人获取这个账号后用这个账号使用电脑，参考[https://www.ithome.com/0/228/192.htm](https://post.smzdm.com/p/akxwkxqk/)</span>
##### 2. 按教程走，主要就是 `限制此用户登录到系统上：“拒绝本地登录”和“拒绝通过远程桌面服务登录”`，避免有意或无意中登录电脑，在电脑中留下无用的用户文件，其它安全配置看情况，一般局域网用户也用不着过于严密

### 五、巴法云

---

#### （一）注册巴法云账号

> - https://cloud.bemfa.com/


---

#### （二）创建主题

 ![](attachment/f24e50567bd0379f33758e446373ddb9a98bac38aedf504891b6331390024eb6.png)

> 1. 主题名字必须为XX001，以001结尾的代表是一个插座设备，后续连接小爱音箱后，对它说打开关闭，巴法云上就会收到on/off的数据，才能进行后续操作。
> 2. 点击昵称就可以修改该主题的名字，这里直接改为电脑。


---

#### （三）添加到小爱同学

##### 1. 在米家app的“我的”菜单项中选择连接其它平台

 ![](attachment/1b91baf838aa7f42fae647cbe44eb63688bb7db29e8fe8121ff9380833502aa8.png)



##### 2. 找到并添加巴法

 ![](attachment/0dfe2b99f0ec93a1c0550d43e3e0f30cf9097e6bd90f43665b740aa357a00597.png)



##### 3. 可以看到有在网页上命名的电脑设备，点击同步设备

 ![](attachment/d0384a2c56040ca76088d0b29629b6e648bdfd12da51b0c567540a3e57ca1e35.png)

---

### 六、iStoreOS

#### （一）创建文件夹

> - 在任意位置创建文件夹，如：  `mkdir /etc/wakeup`

---

#### （二）上传配置文件

- 从仓库中下载并上传config.ini和wakeup.py
  ![](attachment/0295ff457e8b9fc46ba3ccecd9b45e972f2331e5d758d44fe1e79a27705df1ba.png)

---

#### （三）修改配置文件

##### 1. <span class='custom-title-span'>按照说明逐一修改配置文件</span>

##### 2. 巴法云私钥/client_id

- ![](attachment/edbafcef17a510c7a8458197b5457e0db13bece2a8efa74a5a62f44d5aad4dca.png)

##### 3. 主题值/topic

- ![](attachment/ec00e02c5afe8156849b147c4dc67ef840b8883d6697d428f58d8cfafe26287e.png)

##### 4. 设备MAC地址/mac

（1）需要唤醒的设备输入：`ipconfig /all`，找到支持唤醒的网卡的物理地址，注意：如果显示的物理地址为XX-XX-XX-XX-XX-XX，需要将短横杠替换为冒号，XX:XX:XX:XX:XX:XX
![](attachment/b0ac334a7df49d4540ffb28c251eb3249de04ebf240f8f7b362bac89e737e053.png)

##### 5. 远程电脑IP地址/ip

（1）需要唤醒的设备输入：`ipconfig`，根据自己的网卡找到IP地址
 ![](attachment/ccbf0a8dbf57c64da091c6cb5d15499633906e866843e7bc5a910b25a457d3fa.png)

##### 6. 远程SSH用户账号/user

- 设置的用户名

##### 7. 远程SSH用户密码/password

- 设置的用户密码

---

#### （四）安装依赖

```bash
opkg update
opkg install wakeonlan python3 sshpass
```

---

#### （五）开机启动

- 在系统-启动项-本地启动脚本中添加代码，让其开机启动

```bash
nohup /usr/bin/python3 -u /etc/wakeup/wakeup.py 1 > /etc/wakeup/log.txt 2>&1 &
```

 ![](attachment/97d06dbdbc6cd4a8c430f8a23be76cbd9ddf3c5751cab18a2717b2e059826f8e.png)

---

#### （六）计划任务

- 在系统-计划任务中添加任务，作用是每个小时，会kill掉wakeup.py的后台进程，并重新启动一个新的进程，防止长时间掉线。

```bash
0 */1 * * * ps | grep wakeup.py | grep -v grep | awk '{print $1}' | xargs kill -9; nohup /usr/bin/python3 -u /etc/wakeup/wakeup.py 1 > /etc/wakeup/log.txt 2>&1 &
```

 ![](attachment/4d407b391e1c47e842d87406441ca463f04df4e2279c4122b6820174289ed1af.png)

---

#### （七）生效操作

- 重启或来到系统-启动项-启动脚本，ctrl+f 搜索 cron，并点击重启，使计划任务生效

  


---

### 七、更新docker镜像

---

#### （一）创建工作流

##### 触发

```yml
on:
  push:
    tags:
      - "v*"
  workflow_dispatch:
    inputs:
      version:
        description: '输入版本号，格式为X.Y.Z'
        required: true
        default: ''
```
- 打上`V*`标签并推送自动触发操作
  ```bash
  git tag -a v2.0.0 -m "Release version 2.0.0"
  git push --tags
  ```
  - 根据需要修改`v2.0.0`→`va.b.c`和之后的描述
  - `v2.0.0`/`va.b.c`在工作流中将会提取`v2.0.0`/`a.b.c`作为标签打在镜像上，镜像上将会同时有两个tags：`v2.0.0`/`a.b.c`和`latest`

- `workflow_dispatch`是页面的`Run workflow`操作，输入的值保存在参数`github.event.inputs.version`中。

##### 多平台镜像的action

```yml
- name: Set up QEMU
  uses: docker/setup-qemu-action@v3

- name: Set up Docker Buildx
  uses: docker/setup-buildx-action@v3
```

##### 版本的获取

```yml
- name: Use tags or version
  id: final_tags
  run: |
    if [ -z "${{ github.event.inputs.version }}" ]; then
      echo "tags=${{ steps.get_version.outputs.VERSION }}" >> $GITHUB_OUTPUT
    else
      echo "tags=${{ github.event.inputs.version }}" >> $GITHUB_OUTPUT
    fi
```

- 从页面输入或tags中获取版本信息

```yml
- name: Extract metadata (tags, labels) for Docker
  id: meta
  uses: docker/metadata-action@9ec57ed1fcdbf14dcef7dfbe97b2010124a938b7
  with:
    images: yexundao/wakeup_pc
    tags: |
          type=raw,value=${{ steps.final_tags.outputs.tags }}
          type=raw,value=latest
```
- 根据实际参数修改对应的名字，可直接在Docker Hub的镜像发布页查看对应的版本号

##### Docker Hub的登录
```yml
- name: Log in to Docker Hub
  uses: docker/login-action@f4ef78c080cd8ba55a85445d5b36e214a81df20a
  with:
    username: ${{ secrets.DOCKER_USERNAME }}
    password: ${{ secrets.DOCKER_PASSWORD }}
```
- 账号配置
  - Settings→Security→Secrets and variables→Actions→Repository secrets→New repository secret
  - `Name`为参数名，`Secret`为具体的值
  - 用户名参数为：`Name`：`DOCKER_USERNAME`，`Secret`：`Docker Hub的用户名`
  - 密码参数为：`Name`：`DOCKER_PASSWORD`，`Secret`：`Docker Hub的密码`

##### 运行目录的配置
```yml
- name: Build and push
  uses: docker/build-push-action@v5
  with:
    context: ./docker
    file: ./docker/Dockerfile
    platforms: linux/amd64,linux/arm64
    push: true
    tags: ${{ steps.meta.outputs.tags }}
    labels: ${{ steps.meta.outputs.labels }}
```
- 修改运行目录，由于Dockerfile文件是存在于仓库的`/docker/Dockerfile`位置，需要同时修改`context`和`file`为对应的路径。
- 在`platforms: linux/amd64,linux/arm64`中指定了构造平台。

---

### 八、故障排除（更新...）

---

#### （一）iStoreOS的启动项无法启动

- 排查故障
  - 将启动项和计划任务的命令单独放在终端中，查看是否正常启动，是否有报错，再重启尝试。

#### （二）iStoreOS可以唤醒电脑无法关闭电脑
- 排查故障
  - 查看wakeup.py同级目录下是否有log.txt日志生成，查看日志内容排除故障
- 可能原因
  - 如果日志显示：Host '主机IP' is not in the trusted hosts file. 则需要进入iStoreOS终端，在目标PC开机的状态下进行一次ssh连接，连接后自动添加为可信。
    - 如果还是不行，查看是否是类似于(ssh-ed25519 fingerprint SHA256:XbhC....)的提示，去网页，卸载掉ssh相关的软件包，重新安装openssh-client sshpass

#### （三）docker无法唤醒电脑
- 可能原因
  - 进入容器内部，手动执行唤醒代码，如失败则说明可能是docker的网络环境出问题，无法访问到局域网设备，检查并重新设置docker网络配置。

#### （四）docker无法关闭电脑

- 可能原因
  - 可查看日志的第一行，如`cmd_shutdown: sshpass -p 密码 ssh -A -g -o StrictHostKeyChecking=no 用户名@IP 'shutdown /s /t 10'`在容器内手动执行` sshpass -p 密码 ssh -A -g -o StrictHostKeyChecking=no 用户名@IP 'shutdown /s /t 10'`查看是否能够执行成功关闭电脑，可能是网络环境的问题，也可能是Windows的SSH配置问题。
