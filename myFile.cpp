#include "myFile.h"
void mget(char filename[DIRNUM]){
    char ch;
    while((ch=getchar())==' ');
    filename[0]=ch;
    fgets(filename+1,DIRNUM,stdin);
    int i=strlen(filename)-1;
    while(filename[i]==' ' || filename[i]=='\n' || filename[i]=='\r')filename[i--]='\0';
}
int main(){
    string order;
    startsys();
    cout<<username<<":"<<currentdir<<"$ ";
    while(cin>>order){
        char filename[DIRNUM];
        int fd;
        if(order=="exit" || order=="111")break;
        else if(order=="cd"){
            mget(filename);
            my_cd(filename);
        }
        else if(order=="format"){
            my_format();
        }
        else if(order=="mkdir"){
            mget(filename);
            my_mkdir(filename);
        }
        else if(order=="rmdir"){
            mget(filename);
            cout<<filename<<endl;
            my_rmdir(filename);
        }
        else if(order=="ls"){
            my_ls();
        }
        else if(order=="create"){
            mget(filename);
            my_create(filename);
        }
        else if(order=="open"){
            mget(filename);
            int t=my_open(filename);
            cout<<t<<endl;
        }
        else if(order=="close"){
            cin>>fd;
            my_close(fd);
        }
        else if(order=="write"){
            char ch;
            unsigned int offset=0;
            int co=0,type=0;
            fd=0;
            while(1){
                int t=0;
                while((ch=getchar())==' ');
                while(ch!='\n' &&  ch!=' '){
                    t=(t<<1)+(t<<3)+(ch^48);
                    ch=getchar();
                }
                if(co==0) fd=t;
                else if(co==1)offset=(unsigned int)t;
                else type = t;
                co++;
                if(ch=='\n' || co>=3)break;
            }
            my_write(fd,offset,type);
        }
        else if(order=="rm"){
            mget(filename);
            my_rm(filename);
        }
        else if(order=="read"){
            char ch;
            unsigned int offset=0;
            int co=0 ,len=0;
            fd=0;
            while(1){
                int t=0;
                while((ch=getchar())==' ');
                while(ch!='\n' &&  ch!=' '){
                    t=(t<<1)+(t<<3)+(ch^48);
                    ch=getchar();
                }
                if(co==0) fd=t;
                else if(co==1)len=t;
                else offset=(unsigned int)t;
                co++;
                if(ch=='\n' || co>=3)break;
            }
            my_read(fd,len,offset);
        }
        else if(order=="showfat"){
            char ch;
            int co = 0;
            fd = 5;
            while(1){
                int t=0;
                while((ch=getchar())==' ');
                if(ch=='\n')break;
                while(ch!='\n' &&  ch!=' '){
                    t=(t<<1)+(t<<3)+(ch^48);
                    ch=getchar();
                }
                if(co==0) fd=t;
                co++;
                if(ch=='\n' || co>=1)break;
            }
            show_fat(fd);
        }
        else if(order=="show"){
            cout<<"curdir:"<<curdir<<endl;
        }
        else if(order=="showopen"){
            show_openfile();
        }
        else if(order=="cname"){
            char oldname[DIRNUM],newname[DIRNUM];
            cin>>oldname>>newname;
            my_cname(curdir,oldname,newname,1);
        }
        cout<<username<<":"<<currentdir<<"$ ";
    }
    my_exitsys();
    return 0;
}