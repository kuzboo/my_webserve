class myString
{
public:
    myString(const char *cstr = 0);
    myString(const myString &str);

private:
    char *m_data;
}