#include<bits/stdc++.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <omp.h>
#include <arm_neon.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
using namespace std;
#define maxn 5000001
// #define outMAXS 400000000
#define Tnum 8
#define MAX_LOG 2500001 // 输入最大边数
// #define MAX_OUT_DU 30
#define INF 0x3f3f3f3f3f3f
#define T 50
pthread_mutex_t mt;

struct Timer
{
    /*
        计时函数，可以用于多线程内精准计时
    */
    timeval tic, toc;

    Timer()
    {
        gettimeofday(&tic,NULL);
    }

    void stop(const char* name)
    {
        gettimeofday(&toc,NULL);
        printf("%s: %f(s)\n", name, float(toc.tv_sec-tic.tv_sec) + float(toc.tv_usec-tic.tv_usec)/1000000);
    }
};
Timer timer0;

struct node
{
	long long w;
    unsigned int now;
    inline bool operator <(const node &x)const
    //重载运算符把最小的元素放在堆顶（大根堆）
    {
        return w>x.w;//这里注意符号要为'>'
    }
};

struct Edge1
{
    unsigned int u,v,w;
};

struct Edge2
{
    unsigned int v, w;
};

// vector<pair<unsigned int,unsigned int>> Graph[maxn];
Edge2 *G,*denseG;
unsigned int *out_du_num; 
unsigned int *in_du_num; 
// vector<pair<unsigned int,unsigned int>> Graph2[maxn];
// unsigned int size1[maxn];
// unsigned int size2[maxn];

// int res_size[Tnum];
// bool vis[Tnum][maxn];
// unsigned int three_border[Tnum][maxn];
// unsigned int path[Tnum][8];
// string str[maxn];
// pair<unsigned int,unsigned int> lastone[Tnum][maxn];

// vector<unsigned int> inputs_in_num;
// vector<unsigned int> inputs_out_num;
// vector<unsigned int> inputs_menoy;
// vector<unsigned int> id_arr;
unordered_map<unsigned int, unsigned int> ids;
// unordered_map<unsigned int, int> ids2;
unsigned int sort_ids[2*MAX_LOG];
// int node_num=0,record_num=0;
// const unsigned int INF = 0xffffffff;
long long dis[Tnum][maxn];
double delta[Tnum][maxn];
// vector<vector<int> >A[Tnum]; 
// long double A[Tnum][maxn];
double sigma[Tnum][maxn];
// double sigma2[Tnum][maxn];
priority_queue<node>q[Tnum];
char* mmap_buf;
int edge_num,max_id;
// int MAX_OUT_DU = 0;
unsigned int *sum_out_du;
unsigned int *finish;
unsigned int len_double,len_l;
// long long cnt[Tnum];
bool *isdense;
// 读入数据块
struct Read_Block
{
    int begin, end, log_len;
    Edge1 *Log;
    int *out_du,*in_du;
};

Read_Block read_block[Tnum];                   // 读入数据块


inline string myto_string(int value) {

	static const char digits[19] = {
		'9','8','7','6','5','4','3','2','1','0',
		'1','2','3','4','5','6','7','8','9'
	};
	static const char* zero = digits + 9;

	char localbuf[32];
	int i = value;
	char *p = localbuf + 32;
	*--p = '\0' ;
	do {
		int lsd = i % 10;
		i /= 10;
		*--p = zero[lsd];
	} while (i != 0);
	return string(p);
}

// 读数据
void *read(void* arg)
{
    //多线程mmap读入
    Read_Block* blk = (Read_Block*)arg;
    Edge1 *Log = blk->Log;
    char c;
    int i = blk->begin, j = 0;
    unsigned int in_num,out_num,menoy;
    while (i < blk->end)
    {
        in_num = 0, out_num = 0, menoy = 0;
        while ((c = mmap_buf[i++]) != ',')  in_num = (in_num * 10 + c - 48);
        while ((c = mmap_buf[i++]) != ',')  out_num = (out_num * 10 + c - 48);
        while ((c = mmap_buf[i++]) >= 48) menoy = (menoy * 10 + c - 48);
        if (mmap_buf[i] < 48) i++;
		if(menoy == 0) continue;
        Log[j].u = in_num, Log[j].v = out_num, Log[j++].w = menoy;
    }
    blk->log_len = j;
}


void *mapHash(void* arg)
{
    /*
        map映射
    */
    Read_Block* blk = (Read_Block*)arg;
    Edge1* Log = blk->Log;
    blk->out_du = (int*)calloc(max_id, sizeof(int));
    int *out_du = blk->out_du;
	blk->in_du = (int*)calloc(max_id, sizeof(int));
    int *in_du = blk->in_du;
    int len = blk->log_len;
    for (int j = 0; j < len; j++)
    {
        Edge1 temp = Log[j];
        unsigned int u = ids[temp.u], v = ids[temp.v];
        out_du[u]++;
		in_du[v]++;
        Log[j].u = u, Log[j].v = v;
    }
}

inline void load_input(string &input_file){
	//使用mmap读入数据存到mmap_buf中
	struct stat st;
	stat(input_file.c_str(),&st);
	int mmap_len = st.st_size;
	int fd = open(input_file.c_str(), O_RDONLY, S_IRUSR);
	mmap_buf=(char*)mmap(NULL,mmap_len,PROT_READ,MAP_PRIVATE,fd,0);

	//根据线程数将mmap_buf分成多份
	int each_mmap_len = mmap_len / Tnum;
	read_block[0].begin = 0;
	read_block[Tnum-1].end = mmap_len;
	for (int i = 1; i < Tnum; i++)
    {
        int rp = each_mmap_len * i;
        while (mmap_buf[rp++] != '\n');
        read_block[i].begin = rp;
		read_block[i-1].end = rp;
    }

	//初始化read_block中Log变量的空间
    for (int i = 0; i < Tnum; i++)
        read_block[i].Log = (Edge1*)malloc(MAX_LOG * sizeof(Edge1));

	// 多线程读数据
    pthread_t thread[Tnum];
    for (int i = 0; i < Tnum; i++)  pthread_create(&thread[i], NULL, read, read_block + i);
    for (int i = 0; i < Tnum; i++)  pthread_join(thread[i], NULL);
	for (int i = 0; i < Tnum; i++)  edge_num +=  read_block[i].log_len;

	// map映射并排序
	Edge1 e;
	for (int i = 0; i < Tnum; i++)
	{
		Edge1* Log = read_block[i].Log;
		int len = read_block[i].log_len;
		for (int j = 0; j < len; j++)
		{
			Edge1 e = Log[j];
			unsigned int u = e.u, v = e.v;
			if(ids.find(u) == ids.end()) ids[u] = 1, sort_ids[max_id++] = u;
			if(ids.find(v) == ids.end()) ids[v] = 1, sort_ids[max_id++] = v;
		}
	}
	sort(sort_ids, sort_ids + max_id);
	for(unsigned int i = 0; i < max_id; i++) ids[sort_ids[i]] = i;
	// cout<<ids[111357]<<"!!"<<endl;
	//修改read_block结点为排序后的map值
	for (int i = 0; i < Tnum; i++)  pthread_create(&thread[i], NULL, mapHash, read_block + i);
    for (int i = 0; i < Tnum; i++)  pthread_join(thread[i], NULL);


	

	//构建出度数表
	out_du_num = (unsigned int*)calloc(max_id, sizeof(unsigned int));
	in_du_num = (unsigned int*)calloc(max_id, sizeof(unsigned int));
	for(int i = 0; i < Tnum; i++){
        int *out = (read_block + i)->out_du;
		int *in = (read_block + i)->in_du;
        for(int j = 0; j < max_id; j++) {
			out_du_num[j] += out[j];
			in_du_num[j] += in[j];
		}
    }

	isdense = (bool*)calloc(max_id, sizeof(bool));//根据出度数判断结点i是否为菊花点
	sum_out_du = (unsigned int*)calloc(max_id+1, sizeof(unsigned int));
	sum_out_du[0] = 0;
	// for(int i = 0; i < max_id; i++){
	// 	sum_out_du[i+1] = sum_out_du[i] + out_du_num[i];
	// }
	for(int i = 0; i < max_id; i++){
		if(out_du_num[i] > T){
			sum_out_du[i+1] = sum_out_du[i] + out_du_num[i];
			isdense[i] = 1;
		}
		else{
			sum_out_du[i+1] = sum_out_du[i];
		}
	}



	/*
	unsigned int MAX_OUT_DU = 0;
	unsigned int MAX_IN_DU = 0;
	for(int i = 0; i < max_id; i++){
		if(MAX_OUT_DU<out_du_num[i]) MAX_OUT_DU = out_du_num[i];
		if(MAX_IN_DU<in_du_num[i]) MAX_IN_DU = in_du_num[i];
	}
	unsigned int MAX = max(MAX_OUT_DU,MAX_IN_DU)+1;
	unsigned int t_out_du[MAX];
	for(int i = 0; i < MAX; i++) t_out_du[i] = 0;
	unsigned int t_in_du[MAX];
	for(int i = 0; i < MAX; i++) t_in_du[i] = 0;

	for(int i = 0; i < max_id; i++){
		t_out_du[out_du_num[i]]++;
		t_in_du[in_du_num[i]]++;
	}

	unsigned int one_ = 0;
	for(int i = 0; i < max_id; i++){
		if(out_du_num[i]==1&&in_du_num[i]==1) {
			one_++;
		}
	}
	cout<<"one_in_out: "<<one_<<endl;
	cout<<"max_id: "<<max_id<<endl;
	*/


	//构建邻接表
	G = (Edge2*)malloc(max_id * T * sizeof(Edge2));
	denseG = (Edge2*)malloc(sum_out_du[max_id] * sizeof(Edge2));
	unsigned int *tmp = (unsigned int*)calloc(max_id, sizeof(unsigned int));
    int p1;
	// cout<<4<<endl;
    for (int i = 0; i < Tnum; i++)
    {
        Edge1 *Log = read_block[i].Log;
        int len = read_block[i].log_len;
        for(int j = 0; j < len; j++)
        {
            unsigned int u = Log[j].u, v = Log[j].v, w = Log[j].w;
            Edge2 edge = {v, w};
			if(isdense[u]){
				p1 = sum_out_du[u];
            	denseG[p1 + (tmp[u]++)] = edge;
			}
			else{
				p1 = u * T;
				G[p1 + (tmp[u]++)] = edge;
			}
        }
    }

	finish =  (unsigned int*)calloc(max_id, sizeof(unsigned int));
	for(int i = 0; i < max_id; i++){
		// if(finish[i]!=max_id) cout<<"!!"<<endl;
		finish[i] = 1;
	}
	for(unsigned int i = 0; i < max_id; i++){
		if(out_du_num[i]==0){
			finish[i] = 0;
		}
		else if(in_du_num[i]==0&&out_du_num[i]==1){
			finish[i] = 0;
			finish[ G[ i * T].v]++;
		}
	}
	len_double=max_id*sizeof(double);
	len_l=max_id*sizeof(long long);
	// for (int i = 0; i < Tnum; i++){
	// 	memset(dis[i],0x3f,len_l);
	// 	memset(delta[i],0,len_double);
	// }


}
bool cmpsum(pair<double,int>p1,pair<long double,int>p2){
	if(abs(p1.first-p2.first)<=0.0001)return p1.second<p2.second;
	return p1.first>p2.first;
}
pair<double,unsigned int> sum[Tnum][maxn];
inline void save_output(string &output_file){
	// cout<<3<<endl;
	int sfir=0;
	for(unsigned int i=0;i<max_id;i++){
		sum[0][i].second=i;
		for(int tid=1;tid<Tnum;tid++){
			sum[0][i].first+=sum[tid][i].first;
		}
		// sum[0][i].first = floor(sum[0][i].first * 1000.000f + 0.5) / 1000.000f;
	}
	sort(sum[0],sum[0]+max_id,cmpsum);

	string mys;
	
	for(int i=0;i<100;i++){
        char buf[30];
        sprintf(buf,  "%.3f", sum[0][i].first);
		mys+=myto_string(sort_ids[sum[0][i].second])+','+buf+'\n';
	}
	
	long long total_offset = mys.size();

	int fd=open(output_file.c_str(),O_CREAT|O_RDWR,0666);
	1==ftruncate(fd,total_offset);
	char*p=(char*)mmap(NULL,total_offset,PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);
	memcpy(p,mys.c_str(),total_offset);
	// munmap(p,total_offset);
	close(fd);
	// cout<<4<<endl;
}

vector<unsigned int> prv[Tnum][maxn];
// queue<int>quecnt[Tnum];
// double delta2[Tnum][maxn];
// bool viscnt[Tnum][maxn];
// stack<unsigned int>Stk[Tnum];
unsigned int Stk[Tnum][maxn];
unsigned int sizeStk[Tnum];
long long dji_num[Tnum];
// long long cnt_num[Tnum];

void dji(unsigned int s,int tid)
{
	
	unsigned int one_prv = finish[s];
	if(one_prv == 0) return;
	memset(dis[tid],0x3f,len_l);
	memset(delta[tid],0,len_double);
	dis[tid][s]=0;
	sigma[tid][s]=1;
	q[tid].push((node){0,s});
	Edge2 *cur;
    while(!q[tid].empty())//堆为空即为所有点都更新
    {
		
        node x=q[tid].top();
        q[tid].pop();
        unsigned int u=x.now;
        if(dis[tid][u]<x.w) continue; 
        Stk[tid][sizeStk[tid]++] = u;
        dji_num[tid]++;
		if(isdense[u])
			cur = denseG + sum_out_du[u];
		else
			cur = G + u * T;
		unsigned int len = out_du_num[u];
		for(int i=0;i<len;i++)
        {
        	//搜索堆顶所有连边
            unsigned int v=cur[i].v;
            long long newDist=dis[tid][u]+cur[i].w;
            if(dis[tid][v]>newDist)
            {
                dis[tid][v]=newDist;
				// cout<<s<<" "<<v<<endl;
				prv[tid][v].clear();
                sigma[tid][v]=0;
                q[tid].push((node){dis[tid][v],v});
            }
            if(newDist==dis[tid][v]){
                prv[tid][v].emplace_back(u);
                sigma[tid][v]+=sigma[tid][u];
            }
        }
    }
	unsigned int sizestk_ = sizeStk[tid];
    while(sizeStk[tid]){
        unsigned int w=Stk[tid][--sizeStk[tid]];
		double coeff2=(1.0+delta[tid][w])/sigma[tid][w];
		for(unsigned int v:prv[tid][w]){
			delta[tid][v] += sigma[tid][v]*coeff2;
		}   
		sum[tid][w].first += delta[tid][w] * one_prv;
		// cout<<s<<" "<<w<<endl;
			
	}
	sum[tid][s].first -= delta[tid][s];

	// for(int i=0;i<sizestk_;i++){
	// 	unsigned int w=Stk[tid][i];
	// 	dis[tid][w]=INF;
	// 	delta[tid][w]=0;
	// }

}


unsigned int curhead=0;
static void* dji_method(void* c){
	int tid =*(int*)c;
	// cout<<1<<endl;
	while(1){
		pthread_mutex_lock(&mt);
		unsigned int cur=curhead;	
		++curhead;
		pthread_mutex_unlock(&mt);
		if(cur>=max_id)break;
			dji(cur,tid);
	}

}
bool compare_files(string &file1, string &file2){
	//?????1
	ifstream file1_stream;
    file1_stream.open(file1);
    assert(file1_stream.is_open());
	//?????2
	ifstream file2_stream;
    file2_stream.open(file2);
    assert(file2_stream.is_open());

    char c1,c2;
    bool same_flg = true;
    int t = 0;
	int i = 0;
    while (!file1_stream.eof()&&!file2_stream.eof())
    {
        file1_stream>>c1;
        file2_stream>>c2;
		i++;
        if(c1!=c2){
        	same_flg = false;
        	break;
		}
		// if(same_flg == false && t++<20){
		// 	cout<<i<<endl;
		// 		cout<<c1<<" "<<c2<<endl;
		// }

    }
    if(file1_stream.eof()!=file2_stream.eof())
    	same_flg = false;

    file1_stream.close();
    file2_stream.close();
    return same_flg;
}

int main(){
	string num = "juesaibug6";
	string ResultFileName = num + "/result.txt";
	string InputFileName = num + "/test_data.txt";
	string OutputFileName = num + "/tao_out.txt";
	timer0.stop("init");
	Timer timer1;
	load_input(InputFileName);
	timer1.stop("get");

	/*
	string GraphFileName = num + "/graph.txt";
	unsigned int MAX_OUT_DU = 0;
	unsigned int MAX_IN_DU = 0;
	for(int i = 0; i < max_id; i++){
		if(MAX_OUT_DU<out_du_num[i]) MAX_OUT_DU = out_du_num[i];
		if(MAX_IN_DU<in_du_num[i]) MAX_IN_DU = in_du_num[i];
	}

	unsigned int MAX = max(MAX_OUT_DU,MAX_IN_DU)+1;
	unsigned int t_out_du[MAX];
	for(int i = 0; i < MAX; i++) t_out_du[i] = 0;
	unsigned int t_in_du[MAX];
	for(int i = 0; i < MAX; i++) t_in_du[i] = 0;

	for(int i = 0; i < max_id; i++){
		t_out_du[out_du_num[i]]++;
		t_in_du[in_du_num[i]]++;
	}


	cout<<"MAX_IN_DU:"<<MAX_IN_DU<<endl;
	cout<<"MAX_OUT_DU:"<<MAX_OUT_DU<<endl;
	cout<<"max_id:"<<max_id<<endl;

	int rudu_0 = 0;
	for(int i = 0; i < max_id; i++){
		if(in_du_num[i]==0)
			rudu_0++;
	}
	cout<<"rudu_0:"<<rudu_0<<endl;

	string mys;
	mys += "入度  出度  结点数\n";

	// unsigned int t[MAX_IN_DU+1][MAX_OUT_DU+1];
	// for(int i=0;i<=MAX_IN_DU;i++){
	// 	for(int j=0;j<=MAX_OUT_DU;j++){
	// 		t[i][j] = 0;
	// 	}
	// }
	// for(int k=0;k<max_id;k++)
	// 	t[in_du_num[k]][out_du_num[k]]++;

	// for(int i=0;i<=MAX_IN_DU;i++){
	// 	for(int j=0;j<=MAX_OUT_DU;j++){
	// 		if(t[i][j]!=0)
	// 			mys += myto_string(i)+" "+myto_string(j)+" "+myto_string(t[i][j])+"\n";
	// 	}
	// }

	for(int i=0;i<=MAX_IN_DU;i++){
		unsigned int t[MAX_OUT_DU+1];
		for(int j=0;j<=MAX_OUT_DU;j++)
			t[j] = 0;
		for(int k=0;k<max_id;k++){
			if(in_du_num[k]==i)
				t[out_du_num[k]]++;
		}
		for(int j=0;j<=MAX_OUT_DU;j++){
			if(t[j]!=0){
				// cout<<i<<" "<<j<<" "<<t[i][j]<<endl;
				mys += " "+myto_string(i)+"    "+myto_string(j)+"    "+myto_string(t[j])+"\n";
			}
				
		}
	}


	long long total_offset = mys.size();

	int fd=open(GraphFileName.c_str(),O_CREAT|O_RDWR,0666);
	1==ftruncate(fd,total_offset);
	char*p=(char*)mmap(NULL,total_offset,PROT_WRITE|PROT_READ,MAP_SHARED,fd,0);
	memcpy(p,mys.c_str(),total_offset);
	close(fd);

	// 			int t = 0;
	// Edge2 *cur;
	// for(int i=0;i<20;i++){
	// 	cur = G + sum_out_du[i];
	// 	for(int j=0;j<out_du_num[i];j++){
	// 		cout<<j<<" "<<cur[j].v<<endl;
	// 	}
	// }
	*/
	
	Timer timer2;
	unsigned int cs[Tnum];
	pthread_t Td[Tnum];
	
	for(unsigned int i=0;i<Tnum;i++)
    {
        cs[i]=i;
		pthread_create(&Td[i],NULL,dji_method,(void*)(cs+i));
    }
	for(unsigned int i=0;i<Tnum;i++){
		// Td[i].join();
		pthread_join(Td[i],NULL);
	}
	
	timer2.stop("solve");
	Timer timer3;
	save_output(OutputFileName);
	timer3.stop("save");
	Timer timer4;
	cout<<"result is :"<<compare_files(ResultFileName,OutputFileName)<<endl;
	timer4.stop("compare");
	cout<<"max_id : "<<max_id<<endl;
	long long total_dji_num = 0;
	// long long total_cnt_num = 0;
	// long long total_cnt = 0;
	for(unsigned int i=0;i<Tnum;i++) {
		total_dji_num += dji_num[i];
		// total_cnt_num += cnt_num[i];
		// total_cnt += cnt[i];
	}
	cout<<"dji_num :"<<total_dji_num<<endl;
	// cout<<"cnt_num :"<<total_cnt_num<<endl;
	// cout<<"cnt :"<<total_cnt<<endl;
	exit(0);
	return 0;
	
}
