#include<cstdio>
#include<cstring>
#include<string>
#include<algorithm>
#include<iostream>
#include<cstdlib>
#include<queue>
#include<stack>
#include<vector>
#include<iomanip>
#include<cmath>
#include<bitset>
using namespace std;

//修改输出颜色
#define RESET   "\033[0m"
#define BLACK   "\033[30m"
#define RED     "\033[31m"  
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m" 
#define BLUE    "\033[34m"  
#define MAGENTA "\033[35m" 
#define CYAN    "\033[36m"  
#define WHITE   "\033[37m"  
#define BOLDBLACK   "\033[1m\033[30m" 
#define BOLDRED     "\033[1m\033[31m" 
#define BOLDGREEN   "\033[1m\033[32m"
#define BOLDYELLOW  "\033[1m\033[33m" 
#define BOLDBLUE    "\033[1m\033[34m" 
#define BOLDMAGENTA "\033[1m\033[35m"  
#define BOLDCYAN    "\033[1m\033[36m" 
#define BOLDWHITE   "\033[1m\033[37m"  


#define BLOCKSIZE 64    //磁盘块大小
#define END 65535       //FAT中的文件结束标志
#define FREE 0          //FAT中盘块空闲标志
#define MAXOPENFILE 10  //最多同时打开文件个数
#define BLOCKNUM 1000
#define DIRNUM 80 //目录路径普遍长度范围
#define SIZE  (BLOCKSIZE*BLOCKNUM) //磁盘虚拟空间大小

typedef struct FCB{
    char filename[8]; // 文件名
    char exname[3];     //文件扩展名
    unsigned char attribute; //文件属性字段, 0: 目录文件  1: 数据文件
    unsigned short first; //文件起始盘块号
    unsigned int length; //文件长度(字节数)
}fcb;

typedef struct FAT {
    unsigned short id; 
}fat;

typedef struct USEROPEN {
    fcb file_fcb;
    char dir[DIRNUM]; // 打开文件所在路径,以便快速检查指定文件是否已经打开
    char topenfile; //打开表项是否为空, 0:空  1:表示已被占用 
} useropen;

typedef struct BLOCK0 {
    char information[BLOCKSIZE-10]; //存储一些描述信息,如磁盘大小,磁盘块数量等
    unsigned short root;    //根目录文件的起始盘块号
    unsigned char *startblock;  //虚拟磁盘上数据区开始位置
} block0;


const int fatnum = ceil((double)2*BLOCKNUM/BLOCKSIZE);
int curfileorder = -1; //记录当前打开的文件表在文件表数组中的序号
unsigned short openfileindex[MAXOPENFILE];//记录打开的文件的目录项所在磁盘块
const char username[] = "ZhengLi";
unsigned char *myvhard; //指向虚拟磁盘的起始位置
useropen openfiles[MAXOPENFILE];//用户打开的文件表数组
unsigned short curdir; //当前目录的文件描述符
unsigned short curdirlastnum; //当前目录的最后所在盘块号
unsigned int curdirtaken; //当前目录在磁盘上的已占用的空间
char currentdir[DIRNUM]; //记录当前目录的目录名(包括目录的路径)
unsigned char *startp; //记录虚拟磁盘上数据区开始的位置
unsigned char *blockaddr[BLOCKNUM];
fat Fat1[BLOCKNUM],Fat2[BLOCKNUM];
block0 bootdisk;

vector<string> splitFilePath(char *filename,int &mode,const char* del = "/");
void checkDir(unsigned char** tempblock,unsigned short fd,unsigned int &length,fcb *first_fcb,fcb *second_fcb);
void block_init(unsigned short self, unsigned short parent);
vector<unsigned short> findfree(int num=1);
void my_modifylen(unsigned short fd,unsigned short del,int mode);

void my_cname(unsigned short fd,char *oldname,char *newname,int type=0);//更改文件名字
int my_write(int fd,unsigned int offset=0,int type=0);//默认为覆盖写,offset表示偏移量,实现随机写
int my_open(char *filename);
int my_create(char *filename);
int my_read(int fd,int len=0,unsigned int offset=0);
void my_ls();
void startsys();
void fcb_init(fcb *new_fcb, const char* filename, unsigned short first, unsigned char attribute);
void my_format();
void my_exitsys();
void my_close(int fd);
void my_cd(char *dirname);
void my_rm(char *filename);
void my_mkdir(char *dirname);
void my_rmdir(char *dirname);
void show_fat(int fd);
unsigned short findpre(unsigned short x);

bool addDiritem(unsigned short fd,string str,int type){
    fcb first_fcb,second_fcb,temp;
    memset(&temp,0,sizeof(temp));
    unsigned char* tempblock;
    unsigned int length;
    unsigned int curdirtaken0=0;
    unsigned short curdirlastnum0=fd;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);   
    while(length>BLOCKSIZE){
        length-=BLOCKSIZE;
        curdirlastnum0=Fat1[curdirlastnum0].id;
    }
    curdirtaken0=length;
    fcb_init(&temp,(char*)str.c_str(),0,type);
    my_modifylen(fd,sizeof(fcb),1);
    unsigned short t;
    if(BLOCKSIZE-sizeof(fcb)>=curdirtaken0){
        vector<unsigned short> vt = findfree();
        if(vt.size()==0)return false;
        t=vt[0];
        Fat1[t].id = END;
        temp.first = t;
        memcpy(blockaddr[curdirlastnum0]+curdirtaken0,&temp,sizeof(fcb));
        if(fd==curdir)curdirtaken+=sizeof(fcb);
    }
    else{
        vector<unsigned short> newblockv = findfree();
        if(newblockv.size()==0)return false;
        unsigned short newblock = newblockv[0];
        unsigned int prev = BLOCKSIZE-curdirtaken0;
        unsigned int last = sizeof(fcb)-prev;
        Fat1[curdirlastnum0].id = newblock;
        Fat1[newblock].id = END;
        vector<unsigned short> vt = findfree();
        if(vt.size()==0)return false;
        t=vt[0];
        Fat1[t].id = END;
        temp.first = t;
        unsigned char* save = (unsigned char*)malloc(sizeof(fcb));
        memcpy(save,&temp,sizeof(fcb));

        memcpy(blockaddr[curdirlastnum0]+curdirtaken0,save,prev);
        memcpy(blockaddr[newblock],save+prev,last);
        if(fd==curdir)curdirlastnum = newblock;
        if(fd==curdir)curdirtaken = last;
    }
    if(type==0)//如果是目录,就需要初始化第一块
        block_init(t,fd);
    return true;
}

void delDiritem(unsigned short fd,string str1,int type){
    fcb first_fcb,second_fcb,temp;
    unsigned char* tempblock;
    unsigned int length;
    unsigned int curdirtaken0=0;
    unsigned short curdirlastnum0=fd ,cn;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);    
    unsigned int pre,left;
    int j;
    for(j=0;j*sizeof(fcb)<(int)length;j++){
        memcpy(&temp,tempblock+j*sizeof(fcb),sizeof(fcb));
        string str = temp.filename;
        if((int)temp.attribute!=type)continue;
        if((int)temp.attribute==1){
            if(strlen(temp.exname)!=0){
                str+='.';
                str+=temp.exname;
            }
        }
        if(str=="." || str=="..")continue;
        else if(str==str1){
            break;
        }
    }
    bool over = false;
    if((j+1)*sizeof(fcb)==length)over=true;
    while(length>BLOCKSIZE){
        length-=BLOCKSIZE;
        curdirlastnum0=Fat1[curdirlastnum0].id;
    }
    curdirtaken0=length;
    if(curdirtaken0<sizeof(fcb)){
        unsigned short tempnum = curdirlastnum0;
        curdirlastnum0 = findpre(curdirlastnum0);         
        pre = sizeof(fcb)-curdirtaken0;
        left = curdirtaken0;
        
        unsigned char* save = (unsigned char*)malloc(sizeof(fcb));
        memcpy(save,&temp,sizeof(fcb));

        memcpy(save,blockaddr[curdirlastnum0]+(BLOCKSIZE-pre),pre);
        memcpy(save+pre,blockaddr[tempnum],left);
        memcpy(&temp,save,sizeof(fcb));
        curdirtaken0 = BLOCKSIZE-pre;
        Fat1[tempnum].id = FREE;
        Fat1[curdirlastnum0].id = END;
    } 
    else{
        memcpy(&temp,blockaddr[curdirlastnum0]+curdirtaken0-sizeof(fcb),sizeof(fcb));
        curdirtaken0-=sizeof(fcb);
    }
    my_modifylen(fd,sizeof(fcb),-1);
    if(over)return;
    
    int count0 = j*sizeof(fcb)/BLOCKSIZE;
    int offset = j*sizeof(fcb)%BLOCKSIZE;
    cn=fd;
    while(count0>0){
        cn=Fat1[cn].id;
        count0--;
    }
    if(BLOCKSIZE-offset>=sizeof(fcb)){
        memcpy(blockaddr[cn]+offset,&temp,sizeof(fcb));
    }
    else{
        pre = BLOCKSIZE-offset;
        left = sizeof(fcb)-pre;
        unsigned char* save = (unsigned char*)malloc(sizeof(fcb));
        memcpy(save,&temp,sizeof(fcb));

        memcpy(blockaddr[cn]+offset,save,pre);
        cn = Fat1[cn].id;
        memcpy(blockaddr[cn],save+pre,left);
    }
    if(curdir == fd){
        curdirlastnum = curdirlastnum0;
        curdirtaken = curdirtaken0;
    }
}

void StrtocharArray(string s,char temp[DIRNUM]){
    int i;
    for(i=0;i<s.size();i++){
        temp[i]=s[i];
    }
    temp[i]='\0';
}

//查找空盘块
vector<unsigned short> findfree(int num){
    vector<unsigned short> v;
    int co=0;
    for(unsigned short i=2*fatnum+2;i<BLOCKNUM;i++){
        if(Fat1[i].id==FREE){
            v.push_back(i);
            co++;
            if(co==num)break;
        }
    }
    return v;
}

//查找前一个盘块
unsigned short findpre(unsigned short x){
    for(unsigned short i = 2*fatnum+1;i<BLOCKNUM;i++){
        if(Fat1[i].id == x)return i;
    }
    return bootdisk.root;
}

//递归删除指定目录下的文件
void rmdirall(unsigned short fd){
    fcb first_fcb,second_fcb;
    unsigned char* tempblock;
    unsigned int length;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);    
    for(int i=2;i*sizeof(fcb)<length;i++){
        fcb temp;
        memcpy(&temp,tempblock+i*sizeof(fcb),sizeof(fcb));
        if((int)temp.attribute == 0){
            rmdirall(temp.first);
            continue;
        }
        int ha = -1;
        unsigned short cn=temp.first,cnn=cn;
        for(int kk=0;kk<MAXOPENFILE;kk++){
            useropen &cn = openfiles[kk];
            if(!strcmp(cn.file_fcb.filename,temp.filename) && !strcmp(cn.file_fcb.exname,temp.exname)){
                cn.topenfile=0;
                curfileorder=-1;
            }
            if(cn.topenfile==1 && ha==-1)ha=kk;
        }
        if(curfileorder == -1)
            curfileorder = ha;
        while(1){
            if(Fat1[cn].id!=END){
                cn=Fat1[cn].id;
                Fat1[cnn].id=FREE;
                cnn = cn;
            }
            else{
                Fat1[cn].id = FREE;
                break;
            }
        }
    }
    unsigned short cn=fd,cnn=cn;
    while(1){
        if(Fat1[cn].id!=END){
            cn=Fat1[cn].id;
            Fat1[cnn].id=FREE;
            cnn = cn;
        }
        else{
            Fat1[cn].id = FREE;
            break;
        }
    }
}

void show_fat(int fd){
    fd = min(fd,(int)BLOCKNUM/10);
    int line=0;
    cout<<"      ";
    for(int i=0;i<10;i++){
        cout<<setw(8)<<left<<i;
    }
    cout<<endl;
    cout<<"------------------------------------------------------------------------------------"<<endl;
    cout<<setw(5)<<left<<dec<<line<<right<<"|";
    for(int i=0;i<fd*10;i++){
        if(i%10==0 && i!=0){
            cout<<endl;
            line+=10;
            cout<<setw(5)<<left<<dec<<line<<right<<"|";
        }
        cout<<"0x"<<setw(6)<<left<<hex<<Fat1[i].id;
    }
    cout<<endl;
    cout<<dec;
}

void startsys(){
    int i,j;
    myvhard = (unsigned char*)malloc(SIZE);
    for(i=0;i<BLOCKNUM;i++){
        blockaddr[i] = i*BLOCKSIZE + myvhard; //计算每块磁盘块的地址
    }   
    for(i=0;i<MAXOPENFILE;i++){
        openfiles[i].topenfile = 0;//初始化每个文件表
    }

    FILE *fp = fopen("ZL_filesys","rb");
    if(fp==NULL){
        cout<<"The specified file system does not exist."<<endl<<"Start to create and initialize the file system"<<endl;
        my_format();
    }
    else{
        unsigned char* buf = (unsigned char*)malloc(SIZE);
        fread(buf,1,SIZE,fp);
        if(buf==NULL){
            cout<<"empty"<<endl;
            free(fp);
            return ;
        }
        memcpy(myvhard,buf,SIZE);
        memcpy(&bootdisk,blockaddr[0],sizeof(bootdisk));
        memcpy(Fat1,blockaddr[1],2*BLOCKNUM);
        memcpy(Fat2,blockaddr[1+fatnum],2*BLOCKNUM);
        curdir = 2*fatnum+1;
        bootdisk.root = 2*fatnum+1;
        curfileorder = 0;
        free(buf);
        curdirlastnum = curdir;
        unsigned char* tempblock;
        unsigned short fd = curdir;
        fcb first_fcb,second_fcb;
        unsigned int length;
        checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);    
        while(Fat1[curdirlastnum].id!=END){
            curdirlastnum=Fat1[curdirlastnum].id;
        }
        curdirtaken = length%BLOCKSIZE;
        free(tempblock);
    }
    strcpy(currentdir,"~");
    if(fp!=NULL)fclose(fp);
}

//当删除文件、目录或者创建目录、文件后,修改fcb中的length
void my_modifylen(unsigned short fd,unsigned short del,int mode){
    fcb first_fcb,second_fcb;
    unsigned char* tempblock;
    unsigned int length;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
    if(mode==-1){
        first_fcb.length-=del;
    }
    else first_fcb.length+=del;
    memcpy(blockaddr[fd],&first_fcb,sizeof(fcb));
    if(second_fcb.first == fd){
        if(mode==-1){
            second_fcb.length-=del;
        }
        else second_fcb.length+=del;
        memcpy(blockaddr[fd]+sizeof(fcb),&second_fcb,sizeof(fcb));
        return;
    }
    else{
        unsigned short parent = second_fcb.first , cn = parent;
        fcb temp;
        int count0=1;
        memcpy(&temp,blockaddr[parent],sizeof(fcb));
        unsigned char* save = (unsigned char*)malloc(sizeof(fcb));
        memcpy(save,&temp,sizeof(fcb));
        unsigned int len = temp.length,pre,left;
        for(int i=2;i*sizeof(fcb)<len;i++){
            if(i*sizeof(fcb)>count0*BLOCKSIZE){
                left = i*sizeof(fcb)%BLOCKSIZE;
                pre = sizeof(fcb)-left;
                memcpy(save,blockaddr[parent]+(BLOCKSIZE-pre),pre);
                cn = Fat1[parent].id;
                memcpy(save+pre,blockaddr[cn],left);
                memcpy(&temp,save,sizeof(fcb));
                if(temp.first==fd){
                    if(mode==-1){
                        temp.length-=del;
                    }
                    else temp.length+=del;
                    memcpy(save,&temp,sizeof(fcb));
                    memcpy(blockaddr[parent]+(BLOCKSIZE-pre),save,pre);
                    memcpy(blockaddr[cn],save+pre,left);
                    break;
                }
                else{
                    parent = cn;
                    count0++;
                }
            }
            else{
                memcpy(&temp,blockaddr[parent]+(i*sizeof(fcb))%(BLOCKSIZE),sizeof(fcb));
                if(temp.first==fd){
                    if(mode==-1){
                        temp.length-=del;
                    }
                    else temp.length+=del;
                    memcpy(blockaddr[parent]+(i*sizeof(fcb))%(BLOCKSIZE),&temp,sizeof(fcb));
                    break;
                }
            }
        }
    }
    free(tempblock);
}

void block_init(unsigned short self, unsigned short parent){
    fcb temp;
    fcb_init(&temp,".",self,0);
    memcpy(blockaddr[self],&temp,sizeof(fcb));
    fcb_init(&temp,"..",parent,0);
    memcpy(blockaddr[self]+sizeof(fcb),&temp,sizeof(fcb));
}

void fcb_init(fcb *new_fcb, const char* filename, unsigned short first, unsigned char attribute) {
    if((int)attribute==1){
        int i;
        for(i=strlen(filename)-1;i>0;i--){
            if(filename[i]=='.')break;
        }    
        if(i==0 || strlen(filename)<3){
            strcpy(new_fcb->filename, filename);
            new_fcb->exname[0]='\0';
        }
        else{
            char ch[DIRNUM];
            int len0=strlen(filename);
            strncpy(ch,filename,i);
            ch[i]='\0';
            strcpy(new_fcb->filename, ch);
            strncpy(ch,filename+i+1,len0-i-1);
            ch[len0-i-1]='\0';
            strcpy(new_fcb->exname, ch);
        }
    }
    else{
        strcpy(new_fcb->filename, filename);
        new_fcb->exname[0]='\0';
    }
    new_fcb->first = first;
    new_fcb->attribute = attribute;
    if ((int)attribute) new_fcb->length = 0;
    else new_fcb->length = 2*sizeof(fcb);
}

//格式化
void my_format(){
    strcpy(bootdisk.information,"ZhengLi");
    bootdisk.root = 2*fatnum+1;
    bootdisk.startblock = blockaddr[2*fatnum+1];
    for(int i=0;i<2*fatnum+1;i++){
        Fat1[i].id = END;
    }
    for(int i=2*fatnum+1;i<BLOCKNUM;i++){
        Fat1[i].id = FREE;
    }
    for(int i=0;i<BLOCKNUM;i++){
        Fat2[i].id = Fat1[i].id;
    }
    for(int i=0;i<MAXOPENFILE;i++){
        openfiles[i].topenfile=0;
    }
    Fat1[2*fatnum+1].id = END;
    fcb first_diritem,second_diritem;
    fcb_init(&first_diritem,".",2*fatnum+1,0);
    fcb_init(&second_diritem,"..",2*fatnum+1,0);
    memcpy(blockaddr[2*fatnum+1],&first_diritem,sizeof(fcb));
    memcpy(blockaddr[2*fatnum+1]+sizeof(fcb),&second_diritem,sizeof(fcb));
    memcpy(blockaddr[1],Fat1,sizeof(fat)*BLOCKNUM);
    memcpy(blockaddr[1+fatnum],Fat2,sizeof(fat)*BLOCKNUM);
    memcpy(blockaddr[0],&bootdisk,BLOCKSIZE);
    curdir = 2*fatnum+1;
    strcpy(currentdir,"~");
    curdirlastnum = curdir;
    curdirtaken = first_diritem.length;
    cout<<"Successfully format !"<<endl;
}

//找到某个目录的所有目录项/某个文件的所有内容
void checkDir(unsigned char** tempblock,unsigned short fd,unsigned int &length,fcb *first_fcb,fcb *second_fcb){
    memcpy(first_fcb,blockaddr[fd],sizeof(fcb));
    memcpy(second_fcb,blockaddr[fd]+sizeof(fcb),sizeof(fcb));
    *tempblock = (unsigned char*)malloc(first_fcb->length);
    int lenleft = first_fcb->length;
    length = first_fcb->length;
    unsigned short tempfd2 = fd;
    int count0=0;
    while(1){
        memcpy(*tempblock+count0*BLOCKSIZE,blockaddr[tempfd2],min(BLOCKSIZE,lenleft));
        count0++;
        if(Fat1[tempfd2].id==END)break;
        tempfd2 = Fat1[tempfd2].id;
        lenleft -= BLOCKSIZE;
    }
}


//  mode:记录是绝对路径查找还是相对 0 绝对路径  1:相对路径
//  quotation是指路径是否用引号区分 0:没有引号,不能有空格  1:有引号,可以有空格
vector<string> splitFilePath(char *filename,int &mode,const char* del){
    char cn[DIRNUM],cn2[DIRNUM];
    int len = strlen(filename);
    if(filename[0]=='\"' && filename[strlen(filename)-1]=='\"' || filename[0]=='\'' && filename[strlen(filename)-1]=='\''){
        strncpy(cn,filename+1,len-2);
        cn[len-2]='\0';
    }
    else strcpy(cn,filename);
    vector<string>v;
    
        
    if(cn[0]=='/' || cn[0]=='~')
        mode = 0;
    else mode = 1;
    strcpy(cn2,cn);
    char *temp = strtok(cn,del);
    while(temp!=NULL){
        string cnn = temp;
        temp = strtok(NULL,del);
        if(cnn=="~")continue;
        v.push_back(cnn);
    }
    return v;
}

//打开文件
int my_open(char *filename){
    int mode;
    unsigned short fd;
    vector<string>v,name,he;// he存的是各个文件, v存的是当前操作的文件的各级目录
    if(filename[0]=='\"' && filename[strlen(filename)-1]=='\"' || filename[0]=='\'' && filename[strlen(filename)-1]=='\''){
        v.push_back(((string)filename).substr(1,strlen(filename)-2));
        filename = (char*)v[0].c_str();
        v = splitFilePath(filename,mode);
    }
    else{
        string tempstr = filename;
        he = splitFilePath(filename,mode," ");
        if(he.size()==0){
            cout<<"wrong"<<endl;
            return -1;
        }
        else if(he.size()==1){
            v = splitFilePath((char*)tempstr.c_str(),mode);
        }
    }
    name.push_back("~");
    if(mode==1){
        fd = curdir;
        char tempchar[DIRNUM];
        int index = strlen(currentdir);
        tempchar[0]='\"';
        strcpy(tempchar+1,currentdir);
        tempchar[index+1]='\"';
        tempchar[index+2]='\0';
        vector<string>hah = splitFilePath(tempchar,mode);
        for(int i=0;i<hah.size();i++){
            name.push_back(hah[i]);
        }
    }
    else{
        fd = bootdisk.root;
    }
    
    fcb first_fcb,second_fcb;
    fcb temp;
    unsigned char* tempblock;
    unsigned int length;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
    for(int i=0;i<v.size();i++){
        bool mark = false;
        if(v[i]=="."){
            if(i!=v.size()-1)
                continue;
            else{
                mark=1;
            }
        }
        else{
            for(int j=0;j*sizeof(fcb)<length;j++){
                memcpy(&temp,tempblock+j*sizeof(fcb),sizeof(fcb));
                string str  = temp.filename;
                if(i==v.size()-1 && (int)temp.attribute==0)continue;
                if(i<v.size()-1 && (int)temp.attribute==1)continue;
                if((int)temp.attribute==1){
                    if(strlen(temp.exname)!=0){
                        str+=".";
                        str+=(string)temp.exname;
                    }
                }  
                if(v[i]==".."){
                    if(second_fcb.first == fd)
                        break;
                    fd = second_fcb.first;
                    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                    name.pop_back();
                    mark=true;
                    break;
                }   
                if(str==v[i]){
                    if((int)temp.attribute==0){
                        fd = temp.first;
                        checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                        mark = true;
                        name.push_back(v[i]);
                        break;
                    }
                    else if(i==v.size()-1){
                        mark = true;
                        break;     
                    }
                }
            }
        }
        if(!mark)break;
        if(i==v.size()-1){
            string haha = "";
            for(int k=0;k<name.size();k++){
                haha+=name[k];
                if(k<name.size()-1){
                    haha+='/';
                }
            }
            char cn[DIRNUM];
            StrtocharArray(haha,cn);
            int firstempty=-1;
            for(int k=0;k<MAXOPENFILE;k++){
                if(openfiles[k].topenfile==FREE){
                    if(firstempty==-1){
                        firstempty=k;
                    }
                    continue;
                }
                string s1="",s2="";
                s1+=openfiles[k].dir;
                s1+=openfiles[k].file_fcb.filename;
                s1+=openfiles[k].file_fcb.exname;
                s2+=haha;
                s2+=temp.filename;
                s2+=temp.exname;
                if(s1==s2){
                    cout<<"file had already opened"<<endl;
                    curfileorder = k;
                    free(tempblock);
                    return k;
                }
            }
            free(tempblock);
            if(firstempty==-1){
                cout<<"The number of open files has reached the limit"<<endl;
                return -1;
            }
            else{
                StrtocharArray(haha,openfiles[firstempty].dir);
                openfiles[firstempty].file_fcb = temp;
                openfiles[firstempty].topenfile = 1;
                curfileorder = firstempty;
                openfileindex[firstempty]=fd;
                return firstempty;
            }               
        }
    }
    free(tempblock);
    cout<<"The specified file does not exist"<<endl;
    return -1;
}

int my_write(int index,unsigned int offset,int type){
    if(openfiles[index].topenfile==0){
        cout<<"No such index Now"<<endl;
        return -1;
    }
    string s="",s0;
    char ch[SIZE];
    while(fgets(ch,SIZE,stdin)){
        s+=ch;
    }
    if(s.size())s.pop_back();
    useropen &cn = openfiles[index];

    //修改目录项
    int mode;
    unsigned short fd = openfileindex[index] ,fd2;
    fcb first_fcb,second_fcb;
    fcb temp;
    unsigned char* tempblock;
    unsigned int length,templen,need=0,beginLen;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);    
    
    int i;
    for(i=2;i*sizeof(fcb)<length;i++){
        memcpy(&temp,tempblock+i*sizeof(fcb),sizeof(fcb));
        if(temp.attribute==0)continue;
        if(!strcmp(temp.filename,cn.file_fcb.filename)&&!strcmp(temp.exname,cn.file_fcb.exname)){
            break;
        }
    }
    int offset2 = i*sizeof(fcb), myoff = 0 ,offset3;
    while(offset2>BLOCKSIZE){
        offset2-=BLOCKSIZE;
        fd=Fat1[fd].id;
    }   
    offset=min((unsigned int)offset,temp.length); 
    beginLen = (temp.length+BLOCKSIZE-1)/BLOCKSIZE*BLOCKSIZE;
    if(type==0){
        temp.length = min(temp.length,offset);
        templen = temp.length;
        temp.length+=s.size();
        offset3 = offset;
    }
    else if(type==1){
        templen = temp.length;
        temp.length = max(temp.length,(unsigned int)(offset+s.size()));
        offset3 = min(templen,(unsigned int)(offset+s.size()));
    }
    cn.file_fcb.length=temp.length;
    
    if(BLOCKSIZE-offset2>=sizeof(fcb)){
        memcpy(blockaddr[fd]+offset2,&temp,sizeof(fcb));
    }
    else{
        unsigned short pre , left;
        pre = BLOCKSIZE-offset2;
        left = sizeof(fcb)-pre;
        unsigned char* save = (unsigned char*)malloc(sizeof(fcb));
        memcpy(save,&temp,sizeof(fcb));
        memcpy(blockaddr[fd]+offset2,save,pre);
        fd = Fat1[fd].id;
        memcpy(blockaddr[fd],save+pre,left);
    }
    //写入
    fd = cn.file_fcb.first;
    if(beginLen<temp.length){
        int num=1;
        num = (int)(temp.length-beginLen+BLOCKSIZE-1)/BLOCKSIZE;
        vector<unsigned short>blocka = findfree(num);
        if(blocka.size()<num){
            cout<<"Insufficient memory, write failed"<<endl;
            return -1;
        }
        fd2=fd;
        while(Fat1[fd2].id!=END){
            fd2 = Fat1[fd2].id;
        }
        for(int j=0;j<blocka.size();j++){
            Fat1[fd2].id=blocka[j];
            fd2=blocka[j];
            Fat1[fd2].id=END;
        }
    }
    free(tempblock);
    tempblock = (unsigned char*)malloc(SIZE);
    length = cn.file_fcb.length;
    fd2 = fd;
    while(length>BLOCKSIZE){
        memcpy(tempblock+myoff*BLOCKSIZE,blockaddr[fd2],BLOCKSIZE);
        length-=BLOCKSIZE;
        myoff++;
        fd2=Fat1[fd2].id;
    }
    memcpy(tempblock+myoff*BLOCKSIZE,blockaddr[fd2],length);
    fd2 = fd;
    length = cn.file_fcb.length;
    offset2 = offset;
    while(offset>BLOCKSIZE){
        offset-=BLOCKSIZE;
        fd2=Fat1[fd2].id;
    }
    int slen = s.size();
    char *ss = (char *)s.c_str();
    myoff=0; //当前写入字符串写入的下标
    while(myoff<slen){
        int co = min(BLOCKSIZE-(int)offset,slen-myoff);
        memcpy(blockaddr[fd2]+offset,ss+myoff,co);
        if(co==slen-myoff){
            offset+=co;
            break;
        }
        offset=0;
        myoff+=co;
        fd2=Fat1[fd2].id;
    }
    if(offset==BLOCKSIZE && Fat1[fd2].id!=END){
        offset=0;
        fd2=Fat1[fd2].id;
    }
    myoff=0;
    while(offset3<templen){
        int co = min(BLOCKSIZE-offset,templen-offset3);
        memcpy(blockaddr[fd2]+offset,tempblock+offset3,co);
        if(co==templen-offset3){
            offset+=co;
            break;
        }
        offset=0;
        offset3+=co;
        fd2=Fat1[fd2].id;            
    }
    unsigned short cnn=fd2;
    if(Fat1[fd2].id!=END){
        cnn=fd2;
        fd2=Fat1[fd2].id;
        Fat1[cnn].id=END;
        cnn=fd2;
        while(1){
            if(Fat1[fd2].id!=END){
                fd2=Fat1[fd2].id;
                Fat1[cnn].id=FREE;
                cnn = fd2;
            }
            else{
                Fat1[fd2].id=FREE;
                break;
            }
        }
    }
    if(tempblock!=NULL)free(tempblock);
    curfileorder = index;
    return 0;
}

int my_create(char *filename){  
    int mode;
    unsigned short fd;
    vector<string>v,name,he;// he存的是各个文件, v存的是当前操作的文件的各级目录
    if(filename[0]=='\"' && filename[strlen(filename)-1]=='\"' || filename[0]=='\'' && filename[strlen(filename)-1]=='\''){
        v.push_back(((string)filename).substr(1,strlen(filename)-2));
        filename = (char*)v[0].c_str();
        v = splitFilePath(filename,mode);
    }
    else{
        string tempstr = filename;
        he = splitFilePath(filename,mode," ");
        if(he.size()==0){
            cout<<"wrong"<<endl;
            return -1;
        }
        else if(he.size()==1){
            v = splitFilePath((char*)tempstr.c_str(),mode);
        }
        else{
            for(int i=0;i<he.size();i++){
                my_create((char*)he[i].c_str());
            }
            return 0;
        }
            
    }
    name.push_back("~");
    if(mode==1){
        fd = curdir;
        char tempchar[DIRNUM];
        int index = strlen(currentdir);
        tempchar[0]='\"';
        strcpy(tempchar+1,currentdir);
        tempchar[index+1]='\"';
        tempchar[index+2]='\0';
        vector<string> hah = splitFilePath(tempchar,mode);
        for(int i=0;i<hah.size();i++){
            name.push_back(hah[i]);
        }
    }
    else{
        fd = bootdisk.root;
    }
    fcb first_fcb,second_fcb;
    fcb temp;
    unsigned char* tempblock;
    unsigned int length;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
    for(int i=0;i<v.size();i++){
        bool mark = false;
        if(v[i]=="."){
            if(i!=v.size()-1)
                continue;
            else{
                mark=1;
            }
        }
        else{
            for(int j=0;j*sizeof(fcb)<length;j++){
                memcpy(&temp,tempblock+j*sizeof(fcb),sizeof(fcb));
                string str  = temp.filename;
                if(i==v.size()-1 && (int)temp.attribute==0)continue;
                if(i<v.size()-1 && (int)temp.attribute==1)continue;
                if((int)temp.attribute==1){
                    if(strlen(temp.exname)!=0){
                        str+='.';
                        str+=(string)temp.exname;
                    }
                }
                if(v[i]==".."){
                    if(second_fcb.first == fd){
                        break;
                    }
                    fd = second_fcb.first;
                    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                    name.pop_back();
                    mark=true;
                    break;
                }
                else if(str==v[i]){
                    if((int)temp.attribute==1){
                        cout<<"failed!  file already exist"<<endl;
                        return -1;
                    }
                    else {
                        fd = temp.first;
                        checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                        mark = true;
                        name.push_back(str);
                        break;
                    }
                }
            }
        }
        if(!mark && i<v.size()-1){
            cout<<"Path error"<<endl;
            return -1;
        }
        if(i==v.size()-1){
            addDiritem(fd,v[i],1);
        }
    }
    free(tempblock);
    return 0;
}

void my_rm(char *filename){
    int mode;
    unsigned short fd;
    vector<string>v,name,he;// he存的是各个文件, v存的是当前操作的文件的各级目录
    if(filename[0]=='\"' && filename[strlen(filename)-1]=='\"' || filename[0]=='\'' && filename[strlen(filename)-1]=='\''){
        v.push_back(((string)filename).substr(1,strlen(filename)-2));
        filename = (char*)v[0].c_str();
        v = splitFilePath(filename,mode);
    }
    else{
        string tempstr = filename;
        he = splitFilePath(filename,mode," ");
        if(he.size()==0){
            cout<<"wrong"<<endl;
            return ;
        }
        else if(he.size()==1){
            v = splitFilePath((char*)tempstr.c_str(),mode);
        }
        else{
            for(int i=0;i<he.size();i++){
                my_rm((char*)he[i].c_str());
            }
            return;
        }
    }
    name.push_back("~");
    if(mode==1){
        fd = curdir;
        char tempchar[DIRNUM];
        int index = strlen(currentdir);
        tempchar[0]='\"';
        strcpy(tempchar+1,currentdir);
        tempchar[index+1]='\"';
        tempchar[index+2]='\0';
        vector<string> hah = splitFilePath(tempchar,mode);
        for(int i=0;i<hah.size();i++){
            name.push_back(hah[i]);
        }
    }
    else{
        fd = bootdisk.root;
    }
    fcb first_fcb,second_fcb;
    fcb temp;
    unsigned char* tempblock;
    unsigned int length;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
    for(int i=0;i<v.size();i++){
        int mark = 0;
        if(v[i]=="."){
            if(i!=v.size()-1)
                continue;
            else{
                mark=1;
            }
        }
        else{
            for(int j=0;j*sizeof(fcb)<length;j++){
                memcpy(&temp,tempblock+j*sizeof(fcb),sizeof(fcb));
                string str  = temp.filename;
                if(i==v.size()-1 && (int)temp.attribute==0)continue;
                if(i<v.size()-1 && (int)temp.attribute==1)continue;
                if((int)temp.attribute==1){
                    if(strlen(temp.exname)!=0){
                        str+='.';
                        str+=(string)temp.exname;
                    }
                }
                if(v[i]==".."){
                    if(second_fcb.first == fd){
                        break;
                    }
                    fd = second_fcb.first;
                    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                    name.pop_back();
                    mark=1;
                    break;
                }
                else if(str==v[i]){
                    if((int)temp.attribute==1){
                        mark = 1;
                        break;
                    }
                    else{
                        fd = temp.first;
                        checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                        mark = 1;
                        name.push_back(str);
                        break;
                    }
                }
            }
        }
        if(mark==0){
            if(i==v.size()-1)
                cout<<"File doesn't exist"<<endl;
            else{
                cout<<"Path error"<<endl;
            }
        }
        else if(mark==1 && i==v.size()-1){
            int ha = -1;
            for(int kk=0;kk<MAXOPENFILE;kk++){
                useropen &cn = openfiles[kk];
                string str=cn.file_fcb.filename;
                if(strlen(cn.file_fcb.exname)!=0){
                    str+=".";
                    str+=cn.file_fcb.exname;
                }
                if(str==v[i]){
                    cn.topenfile=0;
                    curfileorder=-1;
                }
                if(cn.topenfile==1 && ha==-1)ha=kk;
            }
            if(curfileorder == -1)
                curfileorder = ha;
            unsigned a = temp.first, b = a;
            while(1){
                if(Fat1[a].id==END){
                    Fat1[a].id = FREE;
                    break;
                }
                a=Fat1[a].id;
                Fat1[b].id=FREE;
                b=a;
            }
            delDiritem(fd,v[i],1);
        }
    }
    if(tempblock!=NULL)free(tempblock);
}

int my_read(int index,int len,unsigned int offset){
    if(openfiles[index].topenfile==0){
        cout<<"No such index Now"<<endl;
        return -1;
    }
    string s="";
    useropen &cn = openfiles[index];
    curfileorder = index;
    if(offset>=cn.file_fcb.length)return 0;
    if(len==0)len=cn.file_fcb.length;
    len = min((int)cn.file_fcb.length-(int)offset,len);
    //读入
    int mode;
    unsigned short fd ,fd2;
    fcb temp;
    unsigned char* tempblock;
    char outstr[SIZE];
    unsigned int length,myoff=0;
    length = cn.file_fcb.length;
    fd = cn.file_fcb.first;
    fd2 = fd;
    tempblock = (unsigned char *)malloc(SIZE);
    while(length>BLOCKSIZE){
        memcpy(tempblock+myoff,blockaddr[fd2],BLOCKSIZE);
        length-=BLOCKSIZE;
        myoff+=BLOCKSIZE;
        fd2=Fat1[fd2].id;
    }
    memcpy(tempblock+myoff,blockaddr[fd2],length);
    memcpy(outstr,tempblock+offset,len);
    outstr[len]='\0';
    cout<<outstr<<endl;
    free(tempblock);    
    return 0;
}

void my_ls(){
    unsigned char* tempblock;
    unsigned short fd = curdir;
    fcb curdiritem,first_fcb,second_fcb;
    unsigned int length;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
    int sp = 0;
    bool empty = true;
    for(unsigned i=2;i*sizeof(fcb)<length;i++){
        memcpy(&curdiritem,tempblock+i*sizeof(fcb),sizeof(fcb));
        string name="";
        if((int)curdiritem.attribute==0){
            name=curdiritem.filename;
        }
        else{
            name=curdiritem.filename;
            if(strlen(curdiritem.exname)!=0){
                name+=".";
                name+=(string)curdiritem.exname;
            }
        } 
        for(int j=0;j<name.size();j++){
            if(name[j]==' '){
                name="\'"+name+"\'";
                break;
            }
        }
        if((int)curdiritem.attribute==0){
            cout<<BOLDGREEN<<name<<RESET<<"   ";
        }
        else cout<<name<<"   ";
        empty = false;
        sp++;
        if(sp!=0 && sp%8==0){
            cout<<endl;
            empty=true;
        }
    }
    if(!empty)cout<<endl;
    if(tempblock!=NULL)
        free(tempblock);
}

void my_exitsys(){
    memcpy(blockaddr[0],&bootdisk,sizeof(bootdisk));
    memcpy(blockaddr[1],Fat1,2*BLOCKNUM);
    memcpy(blockaddr[1+fatnum],Fat2,2*BLOCKNUM);
    FILE *fp = fopen("ZL_filesys", "wb");
    fwrite(myvhard, BLOCKSIZE, BLOCKNUM, fp);
    free(myvhard);
    fclose(fp);
}

void my_close(int fd){
    useropen &cn = openfiles[fd];
    if(cn.topenfile == 0){
        cout<<"Index doesn't exist"<<endl;
    }
    else {
        cn.topenfile = 0;
        cout<<"Close Successfully"<<endl;
    }
    curfileorder = -1;
    for(int kk=0;kk<MAXOPENFILE;kk++){
        useropen &cn = openfiles[kk];
        if(cn.topenfile==1){
            curfileorder=kk;
            return ;
        }
    }
}

void show_openfile(){
    int co=0;
    for(int i=0;i<MAXOPENFILE;i++){
        useropen &cn = openfiles[i];
        if(cn.topenfile==1)co++;
    }
    cout<<co<<" file(s) opened"<<endl;
    for(int i=0;i<MAXOPENFILE;i++){
        useropen &cn = openfiles[i];
        if(cn.topenfile==1){
            string s=cn.file_fcb.filename;
            if(strlen(cn.file_fcb.exname)!=0){
                s+=".";
                s+=cn.file_fcb.exname;
            }
            cout<<"data file: ";
            cout<<cn.dir<<"/"<<s<<"   length:"<<cn.file_fcb.length<<endl;
        }
    }
}

void my_cd(char *dirname){
    int mode;
    unsigned short fd;
    vector<string> v ,name;
    v = splitFilePath(dirname,mode);
    name.push_back("~");
    if(mode==1){
        fd = curdir;
        char tempchar[DIRNUM];
        int index = strlen(currentdir);
        tempchar[0]='\"';
        strcpy(tempchar+1,currentdir);
        tempchar[index+1]='\"';
        tempchar[index+2]='\0';
        vector<string> hah = splitFilePath(tempchar,mode);
        for(int i=0;i<hah.size();i++){
            name.push_back(hah[i]);
        }
    }
    else{
        fd = bootdisk.root;
    }
    
    fcb first_fcb,second_fcb;
    unsigned char* tempblock;
    unsigned int length;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
    if(v.size()==0 && mode==0){
        curdir = fd;
        string haha = "~";
        char cn[DIRNUM];
        StrtocharArray(haha,cn);
        strcpy(currentdir,cn);
        curdirlastnum = curdir;
        while(length>BLOCKSIZE){
            length-=BLOCKSIZE;
            curdirlastnum = Fat1[curdirlastnum].id;
        }
        curdirtaken=length;
        return ;
    }
    for(int i=0;i<v.size();i++){
        fcb temp;
        bool mark = false;
        if(v[i]=="."){
            if(i!=v.size()-1)
                continue;
            else{
                mark=1;
            }
        }
        else{
            for(int j=0;j*sizeof(fcb)<length;j++){
                memcpy(&temp,tempblock+j*sizeof(fcb),sizeof(fcb));
                string str  = temp.filename;
                if((int)temp.attribute == 1)continue;
                if(v[i]==".."){
                    if(second_fcb.first == fd){
                        break;
                    }
                    fd = second_fcb.first;
                    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                    name.pop_back();
                    mark=true;
                    break;
                }
                else if(str==v[i]){
                    fd = temp.first;
                    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                    mark = true;
                    name.push_back(v[i]);
                    break;
                }
            }
        }
        if(!mark)break;
        if(i==v.size()-1){
            curdir = fd;
            string haha = "";
            for(int k=0;k<name.size();k++){
                haha+=name[k];
                if(k<name.size()-1){
                    haha+='/';
                }
            }
            char cn[DIRNUM];
            StrtocharArray(haha,cn);
            strcpy(currentdir,cn);
            curdirlastnum = curdir;
            while(length>BLOCKSIZE){
                length-=BLOCKSIZE;
                curdirlastnum = Fat1[curdirlastnum].id;
            }
            curdirtaken=length;
            return ;
        }
    }
    cout<<"Do not exist the specified directory "<<endl;
}

void my_mkdir(char *dirname){
    int mode;
    unsigned short fd;
    vector<string>v,he,name;
    if(dirname[0]=='\"' && dirname[strlen(dirname)-1]=='\"' || dirname[0]=='\'' && dirname[strlen(dirname)-1]=='\''){
        v.push_back(((string)dirname).substr(1,strlen(dirname)-2));
        dirname = (char*)v[0].c_str();
        v = splitFilePath(dirname,mode);
    }
    else{
        string tempstr = dirname;
        he = splitFilePath(dirname,mode," ");
        if(he.size()==0){
            cout<<"wrong"<<endl;
            return ;
        }
        else if(he.size()==1){
            v = splitFilePath((char*)tempstr.c_str(),mode);
        }
        else{
            for(int i=0;i<he.size();i++){
                my_mkdir((char*)he[i].c_str());
            }
            return ;
        }
            
    }
    name.push_back("~");
    if(mode==1){
        fd = curdir;
        char tempchar[DIRNUM];
        int index = strlen(currentdir);
        tempchar[0]='\"';
        strcpy(tempchar+1,currentdir);
        tempchar[index+1]='\"';
        tempchar[index+2]='\0';
        vector<string> hah = splitFilePath(tempchar,mode);
        for(int i=0;i<hah.size();i++){
            name.push_back(hah[i]);
        }
    }
    else{
        fd = bootdisk.root;
    }
    fcb first_fcb,second_fcb,temp;
    unsigned char* tempblock;
    unsigned int length;
    fd = curdir;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
    for(int i=0;i<v.size();i++){
        bool mark = false;
        if(v[i]=="."){
            if(i!=v.size()-1)
                continue;
            else{
                mark=1;
            }
        }
        else{
            for(int j=0;j*sizeof(fcb)<length;j++){
                memcpy(&temp,tempblock+j*sizeof(fcb),sizeof(fcb));
                string str  = temp.filename;
                if((int)temp.attribute==1)continue;
                if(v[i]==".."){
                    if(second_fcb.first == fd){
                        break;
                    }
                    fd = second_fcb.first;
                    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                    name.pop_back();
                    mark=true;
                    break;
                }
                if(str==v[i]){
                    if(i==v.size()-1){
                        cout<<"failed!  Directory exists"<<endl;
                        return ;
                    }
                    else{
                        fd = temp.first;
                        checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                        mark = true;
                        name.push_back(str);
                        break;
                    }
                }
            }
        }
        if(mark)continue;
        if(i<v.size()-1){
            cout<<"Path error"<<endl;
            return ;
        }
        if(i==v.size()-1){
            addDiritem(fd,v[i],0);
        }
    }
    free(tempblock);
}

void my_rmdir(char *dirname){
    int mode;
    unsigned short fd;
    vector<string>v,he,name;
    if(dirname[0]=='\"' && dirname[strlen(dirname)-1]=='\"' || dirname[0]=='\'' && dirname[strlen(dirname)-1]=='\''){
        v.push_back(((string)dirname).substr(1,strlen(dirname)-2));
        dirname = (char*)v[0].c_str();
        v = splitFilePath(dirname,mode);
    }
    else{
        string tempstr = dirname;
        he = splitFilePath(dirname,mode," ");
        if(he.size()==0){
            cout<<"wrong"<<endl;
            return ;
        }
        else if(he.size()==1){
            v = splitFilePath((char*)tempstr.c_str(),mode);
        }
        else{
            for(int i=0;i<he.size();i++){
                my_rmdir((char*)he[i].c_str());
            }
            return;
        }
    }
    name.push_back("~");
    if(mode==1){
        fd = curdir;
        char tempchar[DIRNUM];
        int index = strlen(currentdir);
        tempchar[0]='\"';
        strcpy(tempchar+1,currentdir);
        tempchar[index+1]='\"';
        tempchar[index+2]='\0';
        vector<string> hah = splitFilePath(tempchar,mode);
        for(int i=0;i<hah.size();i++){
            name.push_back(hah[i]);
        }
    }
    else{
        fd = bootdisk.root;
    }
    fcb first_fcb,second_fcb;
    fcb temp;
    unsigned char* tempblock;
    unsigned int length;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
    for(int i=0;i<v.size();i++){
        int mark = 0;
        if(v[i]=="."){
            if(i!=v.size()-1)
                continue;
            else{
                mark=1;
            }
        }
        else{
            for(int j=0;j*sizeof(fcb)<length;j++){
                memcpy(&temp,tempblock+j*sizeof(fcb),sizeof(fcb));
                string str  = temp.filename;
                if((int)temp.attribute==1)continue;
                if(v[i]==".."){
                    if(second_fcb.first == fd){
                        break;
                    }
                    fd = second_fcb.first;
                    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                    name.pop_back();
                    mark=1;
                    break;
                }
                if(str==v[i]){
                    mark=1;
                    if(i!=v.size()-1){
                        fd = temp.first;
                        checkDir(&tempblock,fd,length,&first_fcb,&second_fcb);
                        mark = 1;
                        name.push_back(str);    
                    }
                    break;
                }
            }   
        }
        if(!mark){
            cout<<"Dirname: "<<v[i]<<" does not exist"<<endl;
            continue;
        }
        if(i==v.size()-1){
            rmdirall(temp.first);
            delDiritem(fd,v[i],0);
        }
    }
    free(tempblock);
}

void my_cname(unsigned short fd,char *oldname,char *newname,int type){
    fcb first_fcb,second_fcb,temp;
    unsigned char* tempblock;
    unsigned int length;
    checkDir(&tempblock,fd,length,&first_fcb,&second_fcb); 
    int j=0,markj=-1;
    string str1=oldname,str2=newname;
    for(j=0;j*sizeof(fcb)<(int)length;j++){
        memcpy(&temp,tempblock+j*sizeof(fcb),sizeof(fcb));
        string str = temp.filename;
        if((int)temp.attribute!=type)continue;
        if((int)temp.attribute==1){
            if(strlen(temp.exname)!=0){
                str+='.';
                str+=temp.exname;
            }
        }
        if(str=="." || str=="..")continue;
        if(str==str1 && markj!=-2){
            markj=j;
        }
        if(str2==str){
            markj=-2;
        }
    }   
    if(markj==-1){
        cout<<"File doesn't exist"<<endl;
        return ;
    }
    if(markj==-2){
        cout<<"New filename exists"<<endl;
        return ;
    }
    j=markj;
    unsigned int pre,left,cn=fd;
    int count0 = j*sizeof(fcb)/BLOCKSIZE;
    int offset = j*sizeof(fcb)%BLOCKSIZE;
    while(count0>0){
        cn=Fat1[cn].id;
        count0--;
    }
    int index=0 ,flag=0;
    str1=newname;
    int i;
    for(i=str1.size()-1;i>0;i--){
        if(str1[i]=='.'){
            flag=1;
            break;
        }
    }
    char tempchar[DIRNUM];
    if(flag){
        StrtocharArray(str1.substr(0,i),tempchar);
        strcpy(temp.filename,tempchar);
        StrtocharArray(str1.substr(i+1),tempchar);
        strcpy(temp.exname,tempchar);
    }
    else{
        StrtocharArray(str1,tempchar);
        strcpy(temp.filename,tempchar);
    }
    if(BLOCKSIZE-offset>=sizeof(fcb)){
        memcpy(blockaddr[cn]+offset,&temp,sizeof(fcb));
    }
    else{
        pre = BLOCKSIZE-offset;
        left = sizeof(fcb)-pre;
        unsigned char* save = (unsigned char*)malloc(sizeof(fcb));
        memcpy(save,&temp,sizeof(fcb));
        memcpy(blockaddr[cn]+offset,save,pre);
        cn = Fat1[cn].id;
        memcpy(blockaddr[cn],save+pre,left);
    }    
}

void my_mv(char *source,char *dest){} //文件移动,还没写
