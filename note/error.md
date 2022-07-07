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