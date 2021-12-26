# README
**HDU操作系统实验五-简单的文件系统**

## format

**format    直接输入就行**

> format   格式化文件系统



## cd

**更换当前目录**

> cd /a/b/c   绝对路径查找
>
> cd ~   到主目录
>
> cd  a/b/c   或者  ./a/b/c    相对路径查找
>
> cd  ../  返回上级目录



## ls

**输出当前目录下的文件**

> ls    直接输入即可



## open

**打开指定文件名的文件**

> open /a/b/c   或者 ~/a/b/c  绝对路径查找，打开主目录下指定名字的文件
>
> open  a/b/c   或者  ./a/b/c    相对路径查找，打开当前目录下指定名字的文件

**如果指定文件已打开或者打开成功，返回指定文件在打开文件数组中的访问下标**

**失败则返回-1**



## create

**创建指定文件名的文件（可以有后缀）**

>create /a/b/c   或者 ~/a/b/c  绝对路径查找，主目录下创建指定名字的文件
>
>create  a/b/c   或者  ./a/b/c    相对路径查找，打开当前目录创建指定名字的文件

**如果文件存在或者路径错误，都会有对应的失败提示**



## write

**在打开的文件数组中，指定下标的文件中写入**

> write  0     在下标为0的文件里面，从头、覆盖写入
>
> write  0  n  0  在下标为0的文件里面，偏移量为n、完全覆盖写入
>
> write  0  n  1  在下标为0的文件里面，偏移量为n、部分覆盖写入

 

## read

**在打开的文件数组中，指定下标的文件中读入**

> read  0     在下标为0的文件里面，读入整个文件
>
> read  0  n  在下标为0的文件里面，读入长度为n，偏移量为0的文件
>
> read  0  n  m在下标为0的文件里面，读入长度为n，偏移量为m的文件



## rm

**删除指定名字的文件**

>rm   ./a   或者  a  删除当前目录下的文件a
>
>rm  ~/a  或者   /a  删除主目录下的文件a
>
>rm  ../a  删除上级目录（如果存在）下的文件a

**注：**

**1.可以删除多个文件，用空格隔开**

**2.如果文件名中包含空格，则要用双引号将文件名包裹，且里面的文件名要严格等于所需要查找的文件路径**



## mkdir

**创建指定的目录**

>mkdir   a   b   c  ./d   在当前目录创建目录 a, b, c, d   这里是创建多个
>
>mkdir   ~/a   或者  /a   在主目录下创建目录 a
>
>mkdir   ../a  在上一级目录（如果存在）创建目录 a

**注：**

**1.可以创建多个目录，用空格隔开**

**2.如果文件名中包含空格，则要用双引号将文件名包裹，且里面的文件名要严格等于所需要查找的文件路径**



## rmdir

**创建指定的目录**

>rmdir   a   b   c  ./d     删除当前目录下的目录 a, b, c, d     删除多个
>
>rmdir   ~/a   或者  /a     删除主目录下的目录 a
>
>rmdir   ../a  删除上一级目录（如果存在）下的目录 a

**注：**

**1.可以删除多个目录，用空格隔开**

**2.如果文件名中包含空格，则要用双引号将文件名包裹，且里面的文件名要严格等于所需要查找的文件路径**



## cname

**用于修改当前目录下的数据文件名（目前只实现到这里，后面mv还没实现的，暂时将就用）**

> cname   oldname  newname  将当前目录下的oldname数据文件名修改为newname

****



## close

**关闭打开文件数组里面指定下标的文件**

> close  fd   关闭打开文件数组里面下标为 fd 的文件



## showfat

**显示FAT（十六进制）**

> showfat   显示7行FAT数据，每行10个
>
> showfat   n   显示n行FAT数据，每行10个


## showopen

**显示当前已打开文件的信息**

> showopen   直接使用即可


## show

**显示当前所在磁盘块下标（从0开始）**

> show    直接输入就行



## exit

**退出文件系统**

