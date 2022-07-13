**error: redefinition of ‘void Utils::init(int)’**
void Utils::init(int timeslot) 
{ 
    m_TIMESLOT = timeslot; 
}
void init(int timeslot) {m_TIMESLOT = timeslot;}
error: redefinition of ‘void Utils::init(int)’

**头文件已经设了默认形参 定义的时候不需要再写**
//void Utils::addsig(int sig,void(handler)(int),bool restart=true)
void Utils::addsig(int sig,void(handler)(int),bool restart)

**类的成员函数一定不能漏掉定义 否则一堆未定义的引用**
静态成员函数一定要类外定义
如果没有自定义构造析构，一定不能忘了加{}否则会出现未定义的引用错误

**mysql库手动链接 -lmysqlclient  不是mysql！！！！**

 当url为/时 标识欢迎界面
    if(strlen(m_url)==1)
        strcat(m_url, "judge.html");
**where is judege.html**
 strcpy(m_url_real, "/register.html");
        //将网站目录和/register.html进行拼接，更新到m_real_file中
        /*
const char *doc_root = "/home/yhb/test/my_webServer/root"; //服务器根目录
"/video.html");
"/picture.html");
"/fans.html");
"text/html");