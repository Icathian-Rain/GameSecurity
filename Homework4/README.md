---
班级：网安2002班
学号：U202012043
姓名：范启航
---

# 菁英班作业第4课

对FlappyBird游戏进行逆向分析，修改当局的分数

## 一、使用CE数据查询手动修改分数

### 1、将CE附加到FlappyBird上

![image-20230109170326024](assets/image-20230109170326024.png)

### 2、搜索数值找到存储score的位置

通过score不同值变化进行多次搜索

当前score数值为5

![image-20230109161539136](assets/image-20230109161539136.png)

搜索时只有一个结果，即为Score的存储位置

![image-20230109161741665](assets/image-20230109161741665.png)

将Score值修改为1000

![image-20230109161835328](assets/image-20230109161835328.png)

附加调试器，判断哪个地方代码访问了该地址

![image-20230109161908323](assets/image-20230109161908323.png)

修改成功，死亡时分数值为1000。

![image-20230109161938786](assets/image-20230109161938786.png)

### 3、固定化Score的位置

调试器发现此处地址0999BED0访问了该数据。

![image-20230109162030448](assets/image-20230109162030448.png)

初步判断rdi存放有该对象的基地址，0x64为该score属性偏移地址。

![image-20230109162116129](assets/image-20230109162116129.png)

在此位置设置断点，当程序下次访问时，在此暂停。

重开游戏，让小鸟死亡。

![image-20230109162242239](assets/image-20230109162242239.png)

显示rdi寄存器为9ADCE00,应为此次游戏小鸟对象的基地址。

在内存空间查看这一片地址。

![image-20230109162713374](assets/image-20230109162713374.png)

9ADCE64位置处恰好为1，验证判断正确。

接下来寻找内存中哪个地方存储了小鸟对象的基地址。

搜索数值为9ADCE00的内存位置

![image-20230109163102554](assets/image-20230109163102554.png)

有四处均存储了该数据，先全部添加进备选列表内。

重启游戏，使小鸟Score 为1，暂停。

![image-20230109163303771](assets/image-20230109163303771.png)

四个地址中有2个为0，2个发生了变化，分别查找两个不为0的区域。

0x09ADCE00位置处如下图所示，对应0x64偏移处值为1

![image-20230109163510366](assets/image-20230109163510366.png)

0x09ADC850位置处如下图所示，对应0x64偏移处值也为1

![image-20230109163631009](assets/image-20230109163631009.png)

暂时无法确定，使小鸟死亡，二者值未发生变化。

重启一局，去掉两个0值，重新判断

发现两个地址处结果均为0x9A1C460,此时小鸟score为2，对应0x64偏移处结果也为2，判断这两个地址存放都是小鸟的基地址（存疑）

![image-20230109165000262](assets/image-20230109165000262.png)

重开一局测试

![image-20230109165047262](assets/image-20230109165047262.png)

多次测试后，发现存在一定规律，每两局游戏时，其中一局这两个地址一样的，而其中另一局这两个数值不同，不同的这一局分数为上一局的分数。

![image-20230109165253184](assets/image-20230109165253184.png)

多次测试后，第一个位置变为0，故base1应为临时地址，删去，base2才是真实地址。将score改为777进行测试

![image-20230109165428430](assets/image-20230109165428430.png)

分数刷新后，数值改为778

![image-20230109165439503](assets/image-20230109165439503.png)

## 二、使用CE修改汇编代码

### 1、使用dnspy找到计分规则函数

将Flappy Bird的Assembly-CSharp.dll拖入dnspy中进行反编译

![image-20230109171543159](assets/image-20230109171543159.png)

发现其存在BirdScripts这一主要类

找到score属性字段

![image-20230109171618880](assets/image-20230109171618880.png)

进行交叉查找看那一部分修改了此属性

![image-20230109171656553](assets/image-20230109171656553.png)

发现这两个函数修改了此属性。

![image-20230109171725652](assets/image-20230109171725652.png)

![image-20230109171736875](assets/image-20230109171736875.png)

其中Awake函数为初始化该score为0

而OnTriggerEnter2D为分数增加。

故对OnTriggerEnter2D函数进行分析修改。

### 2、CE中修改OnTriggerEnter2D

在CE中找到该函数的代码段

![image-20230109172017149](assets/image-20230109172017149.png)

rdi为鸟对象实例，而rdi+64即为score地址

将score赋值给rax后，对eax自加，则赋值会rdi+64。

此处应为修改score部分，修改汇编指令inc eax为add eax, 500，即可实现每次通过管道后后加501分

![image-20230109173034830](assets/image-20230109173034830.png)

通过2个管道后，即可实现加1002分

![image-20230109173023078](assets/image-20230109173023078.png)

## 三、使用UnityExplorer调试游戏程序集

### 1、使用MelonLoader对游戏进行修改

![image-20230109173916288](assets/image-20230109173916288.png)

### 2、将UnityExplorer加入mods和libs中

![image-20230109173953309](assets/image-20230109173953309.png)

![image-20230109174002120](assets/image-20230109174002120.png)

### 3、使用UnityExplorer进行修改分数

修改分数为100并应用

![image-20230109174108317](assets/image-20230109174108317.png)

修改分数成功

![image-20230109174147641](assets/image-20230109174147641.png)

## 四、使用程序集注入方式

### 1、编写注入函数

Loader.cs

```c#
namespace InjectDll
{
    public class Loader
    {
        static UnityEngine.GameObject gameObject;
        public static void Load()
        {
            gameObject = new UnityEngine.GameObject();
            gameObject.AddComponent<Cheat>();
            UnityEngine.Object.DontDestroyOnLoad(gameObject);
        }
        public static void Unload()
        {
            UnityEngine.Object.Destroy(gameObject);
        }
    }
}
```

cheat.cs

```c#
public class Cheat : UnityEngine.MonoBehaviour
    {
        private void OnGUI()
        {
            UnityEngine.GUI.Label(new Rect(0, 0, 100, 100), "Hack!\nPress F1: score + 1000\nPress F2: 无敌\nPress F3: 取消无敌");
        }
        public void FixedUpdate()
        {
            if (UnityEngine.Input.GetKeyDown(KeyCode.F1))
            {
                //分数 + 1000
                var bs = UnityEngine.GameObject.FindWithTag("Player").GetComponent<BirdScripts>();
                if (bs != null)
                {
                    bs.score = bs.score + 1000;
                }
            }
            if (UnityEngine.Input.GetKeyDown(KeyCode.F2))
            {
                // 无敌
                var Player = UnityEngine.GameObject.FindWithTag("Player");
                var bs = Player.GetComponent<BirdScripts>();
                Player.GetComponent<Collider2D>().isTrigger = true;
            }
            if (UnityEngine.Input.GetKeyDown(KeyCode.F3))
            {
                //取消无敌
                var Player = UnityEngine.GameObject.FindWithTag("Player");
                var bs = Player.GetComponent<BirdScripts>();
                Player.GetComponent<Collider2D>().isTrigger = false;
            }
        }
    }
```

通过读取按键F1：来搜索Player对象，使对象的score+1000来达到修改分数的目的

### 2、使用sharpMonoInjector进行注入

将注入程序集编译为dll

使用sharpMonoInjector进行注入

![image-20230109204257097](assets/image-20230109204257097.png)

注入成功，左上角显示输出信息

![image-20230109204320496](assets/image-20230109204320496.png)

### 3、测试结果

按下F1后，分数增加1000分，测试成功

![image-20230109204350721](assets/image-20230109204350721.png)

