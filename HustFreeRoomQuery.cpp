#include "httplib.h"
#include <ctime>
#include <sstream>
#include <regex>
#include <Windows.h>
#include <iomanip>
#include <map>
using namespace std;
using std::stringstream;

class Room {
	//例：教室S405的floor为4，number为05，orientation为S，name为S405
	//Room room("S405");
	string floor, number, orientation, name;
public:
	Room(string roomName);
	string generateUrl(string time);
};

Room::Room(string roomName) {
	if (roomName.length() != 4) {
		cout << "错误roomName: \"" << roomName << "\"" << endl;
	}
	orientation = roomName.substr(0, 1);
	floor = roomName.substr(1, 1);
	number = roomName.substr(2, 2);
	name = roomName;
}
string Room::generateUrl(string time) {
	//生成向微校园系统查询该教室某日占用情况的URL
	string _jslbh, _jsmc, _code, _jc, _time, _titleJson;
	string tmp = orientation == "S" ? "C120" : "C121";//微校园系统中用C120和C121代表西十二楼S和西十二楼N
	stringstream url;
	_jslbh = tmp;
	_jslbh.append("0");
	_jslbh.append(floor);
	_jslbh.append("0");
	_jslbh.append(number);
	//S111的jslbh为C12001011
	_jsmc = name + "教室";
	//S111的jsmc为S111教室
	_code = tmp;
	_code.append(",西十二楼");
	_code.append(orientation);
	//S111的code为C120,西十二楼S
	_jc = "7";
	//这个变量是课程节次，这里取值无所谓，但不能没有，否则微校园系统返回错误
	_time = time;
	//当日日期，格式为2022-5-25
	_titleJson = "{'code':'";
	_titleJson.append(tmp);
	_titleJson.append("','time':'");
	_titleJson.append(time);
	_titleJson.append("','name':'西十二楼");
	_titleJson.append(orientation);
	_titleJson.append("'}");
	//S111的titleJson为{'code':'C120','time':'2022-05-25','name':'西十二楼S'}
	url << "http://hub.m.hust.edu.cn/aam/room/queryFreeRoomDetail.action?jslbh=" << _jslbh << "&jsmc=" << _jsmc << "&code=" << _code << "&jc=" << _jc << "&time=" << _time << "&titleJson=" << _titleJson;
	//生成url，包含了微校园空闲教室查询系统的GET请求在url中
	return url.str();
}

string getDate(time_t timestamp = time(NULL))
//返回当前日期，格式2022-05-26
{
	char buffer[20] = { 0 };
	struct tm* info = localtime(&timestamp);
	strftime(buffer, 80, "%Y-%m-%d", info);
	return string(buffer);
}

static int UTF8ToGB2312(const char* utf8, char* gb2312) {
	//转换UTF-8编码为GB2312编码
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, gb2312, len, NULL, NULL);
	if (wstr) delete[] wstr;
	return len;
}

void roomStatDataWashing(string s, string* singleRoomStat) {
	//清洗某教室一天的占用情况数据，一共12节课，按序将每节课的占用情况放到列表stat中
	string pattern = R"(("state":")(.+?)("))";//匹配"state":"上课","JSSB"
	regex r(pattern);
	sregex_iterator rst(s.begin(), s.end(), r), end;//迭代器，end为空迭代器
	for (int i = 0; rst != end && i < 12; i++, rst++) {
		string singleRoomStatI = (*rst)[2];//解引用迭代器得到smatch对象，第二项为第二个被括号括起来的子串(上课)
		singleRoomStat[i] = singleRoomStatI;
	}
}

void getSingleRoomStat(string cookie, string date, string roomName, string* singleRoomStat) {
	//stat为接收数组，第n项为该教室该日第n节课的占用情况
	Room r(roomName);
	string url = r.generateUrl(date);//微校园空闲教室查询系统，此url返回该教室的占用情况的Json数据
	//cout << roomName << "的URL: " << url << endl;//输出生成的url
	httplib::Client cli("http://hub.m.hust.edu.cn");
	httplib::Headers header = {
		{"cookie", cookie},//先在浏览器打开生成的url，手动登录hub账号截获cookie，程序运行会要求输入该cookie以实现模拟登录，微校园空闲教室查询系统不登录不能使用
		{"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9"},
		{"Accept-Encoding", "gzip, deflate"},
		{"Cache-Control", "max-age=0"},
		{"Connection","keep-alive"},
		{"Host", "hub.m.hust.edu.cn"},
		{"User-Agent", "Mozilla/5.0 (iPhone; CPU iPhone OS 13_2_3 like Mac OS X) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/13.0.3 Mobile/15E148 Safari/604.1 Edg/101.0.4951.64"}
	};
	auto res = cli.Get(url.c_str(), header);
	/*
	{"json":{"dataList":[{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"01","state":"上课","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":1,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":12,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"01","state":"上课","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":2,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":11,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"01","state":"上课","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":3,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":10,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"01","state":"上课","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":4,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":9,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":null,"state":"空闲","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":5,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":8,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":null,"state":"空闲","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":6,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":7,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"01","state":"上课","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":7,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":6,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"01","state":"上课","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":8,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":5,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"03","state":"考试","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":9,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":4,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"03","state":"考试","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":10,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":3,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"03","state":"考试","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":11,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":2,"JSLX":"普通教室"},
	{"ZWLS":null,"JSLXMC":null,"SZLC":1,"LX":"03","state":"考试","JSSB":null,"DWBH":"622","XQM":"1","JSMC":"西十二楼S102",
	"JSBH":"C12001002","JSRL":156,"JC":12,"FJSYZT":"1","JXLMC":"西十二楼S","ZWPS":null,"JXLBH":"C120","NUM":1,"JSLX":"普通教室"}],
	"rq":"2022-05-20","title":"您选择了: 西十二楼S \/ S102教室 \/ 2022-05-20","qsjc":"7","str":"2022春季学期- 李翔(U201910499)",
	"jsjc":"7","jsbh":"C12001002","code2":"C120,西十二楼S","jxlbh":"C120","date2":"2022-05-20","jxlmc":"西十二楼S",
	"date":"2022年5月20日占用情况如下:"},"upfile":null}
	*/
	//接收到的数据编码格式为UTF-8，转码为GB2312
	char* body_utf8 = new char[4000], * body_GB2312 = new char[4000];
	int l = (res->body).length();
	if (l > 4000) {
		cout << "服务器返回Json长度大于4000！" << endl;
	}
	int i = 0;
	UTF8ToGB2312((res->body).c_str(), body_GB2312);
	string data = body_GB2312;
	roomStatDataWashing(data, singleRoomStat);
	delete[] body_utf8;
	delete[] body_GB2312;
	body_utf8 = NULL;
	body_GB2312 = NULL;
}
void getAllRoomStat(string cookie, string date, string** allStat, string* roomList, int amount) {
	/// <summary>
	/// 获取所有roomList中所有教室在日期date时的占用情况，存放入allStat
	/// </summary>
	/// <param name="cookie">模拟登录的cookie</param>
	/// <param name="date">查询日期</param>
	/// <param name="allStat">j存放结果的动态二维数组</param>
	/// <param name="roomList">所有要查询的教室列表</param>
	/// <param name="amount">所有教室数量</param>
	int i = 0, j = 0, success = 0, failure = 0;
	string SingleRoomStat[12];
	string failureNames;
	for (i = 0; i < amount; i++) {
		string roomName = roomList[i];
		getSingleRoomStat(cookie, date, roomName, SingleRoomStat);
		if (SingleRoomStat[0] == "") {
			failure++;
			failureNames.append(roomName);
			failureNames.append("  ");
			//cout <<endl<< "无法获取教室" << roomName << "的占用情况！" << endl;
		}
		else {
			success++;
		}
		for (j = 0; j < 12; j++) {
			allStat[i][j] = SingleRoomStat[j];
		}
		for (j = 0; j < 12; j++) {
			SingleRoomStat[j] = "";
		}
		//cout <<"第"<<i+1<<"个教室，教室名称："<<roomList[i] << endl<<endl;
		cout << "\r正在获取所有教室占用情况 (" << i + 1 << "/" << amount << ")";
	}
	cout << endl << endl << "已获取全部教室占用情况，成功" << success << "个，失败" << failure << "个：";
	if (failure > 0) {
		cout << failureNames << endl;
	}
	else {
		cout << endl;
	}
	cout << "注：由于微校园服务器的限制，西十二可能有5个左右教室信息无法获取，这是正常的" << endl;
}



int generateRoomList(string* roomList, int amount) {
	int i = 0, j = 0, sub = 0;
	stringstream roomName;
	cout << endl << "所有可用的西十二教室：" << endl;
	//S101-S512
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 9; j++) {
			if (i == 0 && j >= 4 && j <= 7) {
				continue;//一楼没有5~8教室
			}
			roomName << "S" << i + 1 << '0' << j + 1;
			roomList[sub] = roomName.str();//记得排除总控制室和S511音乐教室;
			roomName.str("");
			cout << roomList[sub] << " ";
			sub++;
		}
		for (j = 9; j < 12; j++) {
			if (i == 4 && j == 10) {
				continue;//排除S511音乐教室
			}
			roomName << "S" << i + 1 << j + 1;
			roomList[sub] = roomName.str();
			roomName.str("");
			cout << roomList[sub] << " ";
			sub++;
		}
		cout << endl;
	}
	//N101-N512
	for (i = 0; i < 5; i++) {
		for (j = 0; j < 9; j++) {
			if (i == 0 && j >= 4 && j <= 7) {
				continue;//一楼没有5~8教室
			}
			if (i == 1 && (j == 4 || j == 5)) {
				continue;//排除总控制室N205、N206
			}
			roomName << "N" << i + 1 << '0' << j + 1;
			roomList[sub] = roomName.str();
			cout << roomList[sub] << " ";
			roomName.str("");
			sub++;
		}
		for (j = 9; j < 12; j++) {
			roomName << "N" << i + 1 << j + 1;
			roomList[sub] = roomName.str();
			cout << roomList[sub] << " ";
			roomName.str("");
			sub++;
		}
		cout << endl;
	}
	cout << "注：西十二楼一楼没有5-8教室，N205、N206为总控制室，S511为音乐教室，不对外开放。" << endl;
	cout << "全部教室数量：" << sub << endl;
	if (sub != amount) {
		cout << "length of roomList is unequal to constant roomAmount!" << endl;
	}
	return sub;
}
void displayResult(int num, string* desiredRooms) {
	int i, j;
	for (i = 0; i < num / 12 + 1; i++) {
		for (j = 0; j < 12 && i * 12 + j < num; j++) {
			cout << setw(6) << desiredRooms[i * 12 + j];
		}
		cout << endl;
	}
}

int searchForRoom(string* roomList, string** allStat, string* desiredRooms, int amount, int a = 1, int b = 12, string order = "GetRoomFreeWithin") {
	if (a > b) {//如果a>b就互换值
		int tmp = a;
		a = b;
		b = tmp;
	}
	if (desiredRooms[0] != "") {
		cout << endl << "数组desiredRooms初始不为空！" << endl;
		cout << "desiredRooms[0]: " << desiredRooms[0] << endl;
	}
	int i = 0, j = 0;
	int k = 0;
	int flag = 1;//flag=1代表当前教室符合条件
	for (i = 0; i < amount; i++) {
		for (j = a - 1; j <= b - 1; j++) {
			//根据不同命令，符合条件不同
			if (order == "GetRoomFreeWithin") {
				flag = allStat[i][j] == "空闲";
			}
			if (order == "GetRoomJustFreedWithin") {
				flag = allStat[i][a - 2] != "空闲" && allStat[i][j] == "空闲";
			}
			if (order == "GetRoomFreeFromUntil") {
				if (j == b - 1) {
					break;
				}
				flag = allStat[i][j] == "空闲"&&allStat[i][b - 1] != "空闲";
			}
			if (order == "GetRoomJustFreedFromUntil") {
				if (j == b - 1) {
					break;
				}
				flag = allStat[i][a - 2] != "空闲" && allStat[i][j] == "空闲" && allStat[i][b - 1] != "空闲";
			}
			if (flag == 0) {
				break;
			}
		}
		if (flag == 1) {//符合条件
			desiredRooms[k++] = roomList[i];
		}
		flag = 1;
	}
	return k;//符合条件的教室数量
}

int main() {
	const int roomAmount = 109;//西十二楼除总控制室(N205、N206)和音乐教室S511外，共有教室109间
	int i = 0, j = 0;
	string currentDate = getDate();//格式2022-05-20
	string inputDate;//手动输入指定日期
	string cookie;//已登录hust账户后的cookie信息，需手动获取

	string** allStat;//动态申请二维数组，存放所有教室的占用情况, 每行代表一个教室，每行12列，第n列代表第n节课的占用情况
	allStat = new string * [roomAmount];
	for (i = 0; i < roomAmount; i++) {
		allStat[i] = new string[12];
	}

	string roomList[roomAmount];//所有教室名称列表
	//让用户手动登录并取得cookie
	cout << "欢迎使用！\n微校园需要登录hub账号才能查询教室情况，请手动复制下面的网址用浏览器打开，然后打开浏览器控制台，\n将会跳转到hub统一身份认证系统，登录您的hub账号，找到对该URL的http请求，复制请求标头中cookie项的值（这对微校园服务器代表了您的已登录状态）." << endl;
	cout << "cookie应该是类似这样的形式：BIGipServerpool_122.205.11.43_80=261126410.22811.0000; JSESSIONID=VLgOzxJzqz5PHSBgVJseUjhJhTcMSZvCcQ7hfi3qdpUUWWlUEfB!148362727" << endl;
	cout << endl <<"警告：" << endl;
	cout << "请您注意，请勿将您获取的cookie泄露给他人，否则可能面临被盗取hub账号的风险。" << endl;
	cout << "并且，如果您不能信任本程序的来源或有任何理由怀疑本程序是被人篡改过的版本，也请不要继续输入您的cookie!" << endl;
	cout << "本程序开源，请获取本程序源代码自行审阅后编译使用。" << endl;
	cout << "本程序仅供方便查找自习教室使用，请勿将本程序用于任何不当用途。作者不对任何因对本程序的不当使用而产生的后果负责。" << endl;
	cout << "谢谢理解！" << endl;
	cout << endl << "网址：http://hub.m.hust.edu.cn/aam/room/queryFreeRoomDetail.action" << endl << endl;
	//手动输入截获的cookie，模拟登录
	cout << "请输入cookie: ";
	getline(cin, cookie);
	cout << endl << "您输入的cookie: " << cookie << endl;
	//生成所有教室名称列表
	generateRoomList(roomList, roomAmount);

	//输入查询的日期，直接回车则使用当日日期
	cout << endl << "请输入要查询的日期，格式"<<currentDate<<"，直接回车则默认查询日期为今日：";
	getline(cin, inputDate);
	if (inputDate == "") {
		inputDate = currentDate;
	}
	cout << "您要查询的日期为：" << inputDate << endl;
	cout << endl << "开始查询，如果程序错误，请检查您输入的cookie与日期格式是否正确。" << endl;
	//string stat[12];
	//getSingleRoomStat(cookie, date, roomList[1], stat);//测试用
	getAllRoomStat(cookie, inputDate, allStat, roomList, roomAmount);

	//Print all-room status
	cout << endl << setw(10) << "教室\\节次";
	for (i = 0; i < 12; i++) {
		cout << setw(6) << i + 1;
	}
	cout << endl;
	for (i = 0; i < roomAmount; i++) {
		cout << setw(10) << roomList[i];
		for (j = 0; j < 12; j++) {
			cout << setw(6) << allStat[i][j];
		}
		cout << endl;
	}

	//支持的所有命令
	map<string, string> orderListStr;//注意map中元素是自动按key升序排列的，不是按插入顺序排列
	map<string, regex> orderListRegex;
	orderListStr["GetRoomFreeWithin"] = R"(^\s*freef([0-9]*)t([0-9]*)\s*$)";//匹配命令freef{a}t{b}; a, b为1-12的整数. 查找在节次[a,b]区间内全部空闲的教室，意为free from a to b.
	orderListStr["GetRoomFreeTo"] = R"(^\s*freet([0-9]*)\s*$)";//匹配命令freet{b}; a为1-12的整数. 查找在节次[1,b]区间内全部空闲的教室，意为free to b.
	orderListStr["GetRoomFreeFrom"] = R"(^\s*freef([0-9]*)\s*$)";//匹配命令freef{a}; a为1-12的整数. 查找在节次[a,12]区间内全部空闲的教室，意为free from a.
	orderListStr["GetRoomJustFreedWithin"] = R"(^\s*freedf([0-9]*)t([0-9]*)\s*$)";//匹配命令freedf{a}t{b}; a, b为2-12的整数. 查找在节次[a,b]区间内空闲，但在节次a-1被占用的教室, 意为freed from a to b.
	orderListStr["GetRoomJustFreedFrom"] = R"(^\s*freedf([0-9]*)\s*$)";//匹配命令freedf{a}; a为2-12的整数. 查找在节次[a,12]区间内空闲，但在节次a-1被占用的教室, 意为freed from a.
	orderListStr["GetRoomFreeFromUntil"] = R"(^\s*freef([0-9]*)u([0-9]*)\s*$)";//匹配命令freef{a}u{b}; a为1-11的整数, b为2-12的整数. 查找在节次[a,b-1]区间内空闲, 但在节次b被占用的教室，意为free from a until b.
	orderListStr["GetRoomFreeUntil"] = R"(^\s*freeu([0-9]*)\s*$)";//匹配命令freeu{b}; b为2-12的整数. 查找在节次[1,b-1]区间内空闲, 但在节次b被占用的教室, 意为free until b.
	orderListStr["GetRoomJustFreedFromUntil"] = R"(^\s*freedf([0-9]*)u([0-9]*)\s*$)";//匹配命令freedf{a}u{b}; a, b为2-12的整数. 查找在节次[a,b-1]区间内空闲，但在节次a-1和b被占用的教室, 意为freed from a until b.

	//生成匹配相应命令的regex对象的列表
	i = 0;
	int orderLen = orderListStr.size();
	for (auto it = orderListStr.begin(); it != orderListStr.end(); it++, i++) {
		string key = it->first;
		regex r(it->second);
		orderListRegex[key] = r;
	}

	//使用帮助
	cout << endl << "支持的命令：" << endl;
	cout << setiosflags(ios::left);
	cout <<setw(20)<<"命令"			<<setw(30) << "查找对象" << endl;
	cout <<setw(20)<<"freef{a}t{b}"	<<setw(30) << "[a, b]空闲的教室, 意为free from a to b." << endl;
	cout <<setw(20)<<"freet{b}"		<<setw(30) <<"[1, b]空闲的教室, 意为free to b." << endl;
	cout <<setw(20)<<"freef{a}"		<<setw(30) <<"[a, 12]空闲的教室, 意为free from a." << endl;
	cout <<setw(20)<<"freedf{a}t{b}"<<setw(30) <<"[a, b]空闲，但在节次a - 1被占用教室, 意为freed from a to b." << endl;
	cout <<setw(20)<<"freedf{a}"	<<setw(30) <<"[a, 12]空闲，但在节次a - 1被占用的教室, 意为freed from a." << endl;
	cout <<setw(20)<<"freef{a}u{b}"	<<setw(30) <<"[a,b-1]空闲, 但在节次b被占用的教室, 意为free from a until b." << endl;
	cout <<setw(20)<<"freeu{b}"		<<setw(30) <<"[1,b-1]空闲, 但在节次b被占用的教室, 意为free until b." << endl;
	cout <<setw(20)<<"freedf{a}u{b}"<<setw(30) <<"[a,b-1]空闲，但在节次a-1和b被占用的教室, 意为freed from a until b." << endl;
	cout << "例：\n输入freedf7，返回所有第6节有课且[7,12]空闲的教室" << endl;

	while (1) {//每循环一次获取一次输入命令并执行，给出结果，再进入下一轮循环等待命令
		string order;//freef1t12
		string key;//GetRoomFreeWithin
		smatch orderMatched;//order="freef1t12", orderMatched.size()=3, orderMarched[0]="freef1t12", orderMatched[1]=1, orderMatched[2]=12
		string desiredRooms[roomAmount];//符合搜索条件的教室名称列表
		int num = 0;//num为执行命令后得到的符合条件的教室数量
		int a = 0, b = 0;//双参数命令的两个参数，起始节次与终止节次
		int t = 0;//单参数命令的参数，表示起始或终止节次
		//获取用户输入命令
		for (i = 0; i < 72; i++)cout << "-";//分割线
		cout << endl;
		cout << "请输入命令：";
		getline(cin, order);

		//遍历判断输入命令类型
		i = 0;
		for (auto it = orderListRegex.begin(); i < orderLen; i++, it++) {
			key = it->first;//使order等于当前正则表达式在orderListRegex中的key
			regex orderRegex = it->second;
			bool matched = regex_match(order, orderMatched, orderRegex);
			if (matched) {
				//获取输入命令的参数
				switch (orderMatched.size()) {
				case 2://代表输入的命令有一个参数，如freef1, 参数为t=1
					t = atoi(orderMatched[1].str().c_str());//字符串转换为整数
					break;
				case 3://freef1t12, 两个参数a=1和b=12
					a = atoi(orderMatched[1].str().c_str());//字符串转换为整数
					b = atoi(orderMatched[2].str().c_str());//字符串转换为整数
					if (a > b) {//如果a>b就交换
						int tmp = a;
						a = b;
						b = tmp;
					}
					break;
				}
				break;
			}
			else {
				key = "";
			}
		}

		//执行与输入命令对应的函数
		if (key == "GetRoomFreeWithin") {
			if (a < 1 || a>12 || b < 1 || b > 12) {
				cout << "请检查您的命令，命令freef{a}t{b}意为查找在[a,b]闭区间内空闲的教室，a应大于等于1，b应小于等于12. \n注：如果希望查找第a-1节课或b+1节课被占用的教室，请查阅说明使用其他命令." << endl;
			}
			else {
				num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, a, b);
				cout << "找到在[" << a << "," << b << "]区间空闲的教室" << num << "间：" << endl;
			}
		}

		else if (key == "GetRoomFreeTo") {
			if (t > 12 || t < 1) {
				cout << "请检查您的命令，命令freet{t}意为查找在[1,t]闭区间内空闲的教室，t应小于等于12，大于等于1" << endl;
				cout << "注：该命令查找出的教室在第t+1节课仍可能为空闲，如果需要查找在第t+1节课即被占用的教室，请使用命令freeu{t}" << endl;
			}
			else {
				num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, 1, t);
				cout << "找到在[1," << t << "]区间空闲的教室" << num << "间：" << endl;
			}
		}

		else if (key == "GetRoomFreeFrom") {
			if (t < 1 || t>12) {
				cout << "请检查您的命令，命令freef{t}意为查找从第t节课开始剩余全天空闲的教室，t应大于等于1，小于等于12." << endl;
				cout << "注：该命令查找出的教室在第t-1节课也可能为空闲，如果需要查找在第t-1节课处于被占用状态的教室，请使用命令freedf{t}" << endl;
			}
			else {
				num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, t, 12);
				cout << "找到在[" << t << ",12]区间空闲的教室" << num << "间：" << endl;
			}
		}

		else if (key == "GetRoomJustFreedWithin") {
			if (a < 1 || a>12 || b < 1 || b>12) {
				cout << "请检查您的命令，命令freedf{a}t{b}意为查找在第a-1节课被占用，但在[a,b]闭区间内空闲的教室，a应大于等于1，b应小于等于12.（命令freedf1t{b}会被自动转换为freef1t{b}）" << endl;
			}
			else {
				if (a == 1) {
					num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, 1, b, "GetRoomFreeWithin");
					cout << "找到在[1," << b << "]区间空闲的教室" << num << "间：" << endl;
				}
				else {
					num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, a, b, "GetRoomJustFreedWithin");
					cout << "找到在第" << a - 1 << "节被占用，但在[" << a << ", " << b << "]区间空闲的教室" << num << "间：" << endl;
				}
			}
		}

		else if (key == "GetRoomJustFreedFrom") {
			if (t < 1 || t>12) {
				cout << "请检查您的命令，命令freedf{t}意为查找在第t-1节课被占用，但在从第t节课开始剩余全天空闲的教室，t应大于等于1，小于等于12（命令freedf1会被自动转换为freef1）." << endl;
			}
			else {
				if (t == 1) {//如果输入命令freedf1，就等价于freef1，否则结果为0
					num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, 1, 12, "GetRoomFreeWithin");
					cout << "找到在[" << "1" << "," << "12" << "]区间空闲的教室" << num << "间：" << endl;
				}
				else {
					num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, t, 12, "GetRoomJustFreedWithin");
					cout << "找到在第" << t - 1 << "节被占用，但在[" << t << ", " << "12" << "]区间空闲的教室" << num << "间：" << endl;
				}
			}
		}

		else if (key == "GetRoomFreeFromUntil") {
			if (a < 1 || a>12 || b < 1 || b>12) {
				cout << "请检查您的命令，命令freef{a}u{b}意为查找在[a,b-1]闭区间空闲，但在第b节课被占用的教室，a应大于等于1，b应小于等于12." << endl;
				cout << "注：命令freef{a}u12返回的是在[a,11]空闲，但在第12节课被占用的教室，要查找剩余全天都空闲的教室，请使用命令freef{a}t12或freef{a}" << endl;
			}
			num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, a, b, "GetRoomFreeFromUntil");
			cout << "找到在[" << a << "," << b - 1 << "]区间空闲，但在第" << b << "节被占用的教室" << num << "间：" << endl;
		}

		else if (key == "GetRoomFreeUntil") {
			if (t < 1 || t>12) {
				cout << "请检查您的命令，命令freeu{t}意为查找在[1,t-1]闭区间空闲，但在第b节课被占用的教室，t应大于等于1，小于等于12." << endl;
				cout << "注：命令freeu12返回的是在[1,11]空闲，但在第12节课被占用的教室，要查找全天都空闲的教室，请使用命令freef1t12或freef1或freet12" << endl;
			}
			num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, 1, t, "GetRoomFreeFromUntil");
			cout << "找到在[" << "1" << "," << t - 1 << "]区间空闲，但在第" << t << "节有课的教室" << num << "间：" << endl;
		}

		else if (key == "GetRoomJustFreedFromUntil") {
			if(a < 1 || a>12 || b < 1 || b>12) {
				cout << "请检查您的命令，命令freedf{a}u{b}意为查找在[a,b-1]闭区间空闲，但在第a-1和第b节课被占用的教室, a,b应大于等于1，小于等于12." << endl;
				cout << "注：命令freedf1u{b}将自动被转换为freef1u{b}, 另请注意freedf{a}u{12}返回的是在第a-1和第12节课被占用的教室，如果想查找从第a节课开始剩余全天都空闲的教室，请使用命令freedf{a}" << endl;
			}
			num = searchForRoom(roomList, allStat, desiredRooms, roomAmount, a, b, "GetRoomJustFreedFromUntil");
			cout << "找到在[" << a << "," << b - 1 << "]区间空闲，但在第" << a - 1 << "和第" << b << "节课被占用的教室" << num << "间：" << endl;
		}

		else {
			cout << "您输入的命令未被定义，请检查输入或查阅说明。" << endl;
		}

		//展示结果
		displayResult(num, desiredRooms);
	}

	//释放allStat
	for (i = 0; i < roomAmount; i++) {
		delete[] allStat[i];
		allStat[i] = NULL;
	}
	delete[] allStat;
	allStat = NULL;
}
