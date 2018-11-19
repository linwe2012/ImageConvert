#define _CRT_SECURE_NO_WARNINGS
#if 0
#include <stdio.h>
#include <stdio.h>
#include <dvec.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
enum ColorSpace {
	rgb,
	yuv,
	gray,
	binary,
};
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef int32_t LONG;
class Image {
	typedef struct tagBITMAPFILEHEADER
	{
		WORD bfType; //固定为0x4d42; 'BW'
		DWORD bfSize; //文件大小
		DWORD bfReserves;
		//WORD bfReserved1; //保留字，不考虑
		//WORD bfReserved2;  //保留字，同上
		DWORD bfOffBits; //实际位图数据的偏移字节数，即前三个部分长度之和
	} BITMAPFILEHEADER;

	typedef struct tagBITMAPFILEINFO
	{
		DWORD biSize;//(第14、15、16、17字节)
		//28 00 00 00  即0x28(十进制40)，位图信息头占40字节

		LONG  biWidth;// (第18、19、20、21字节)
		//0a 00 00 00  即0x0a(十进制10)，位图的宽度是10像素

		LONG  biHeight; //(第22、23、24、25字节)
		//0a 00 00 00  即0x0a(十进制10)，位图的高度是10像素

		DWORD biPlanes_biBitCount; //combined stuff for compatibility
		//WORD  biPlanes; // (第26、27字节)
		//01 00        即0x01(十进制1)，目标设备级别，为1

		//WORD  biBitCount; // (第28、29字节) 采用颜色位数，可以是1，2，4，8，16，24，新的可以是32
		//18 00       即0x18(十进制24) 图标是24位图

		DWORD biCompression; // (第30，31，32，33字节)
		//00 00 00 00  即十进制0 ，表示图片未压缩

		DWORD biSizeImage; //(第34、35、36、37字节)
			//42 01 00 00 即0x00000142(十进制322) 图片中像素数据的大小(整个图片的大小减去位图
			//文件头和位图信息头的大小，376 - 14 - 40 = 322)

		LONG biXPelsPerMeter; // (第38、39、40、41字节)
		//20 2E 00 00 即0x00002E20(十进制11808) 位图水平分辨率，每米11808像素

		LONG biYPelsPerMeter; // (第42、43、44、45字节)
		//20 2E 00 00 即0x00002E20(十进制11808) 位图垂直分辨率，每米11808像素

		DWORD biClrUsed;  // (第46、47、48、49字节)
		//00 00 00 00 即0，位图使用颜色表中的颜色数是0，
		DWORD biClrImportant; // (第50、51、52、53字节)
		//00 00 00 00 即 0 位图显示过程中重要的颜色数是0
	} BITMAPFILEINFO;
	typedef uint8_t image_t;
	ColorSpace status;
	//std::vector<uint8_t> image;
	image_t *image;
	double image_color;
public:
	Image();
	int read(std::string filePath);
	int write(std::string filePath);
	typedef union tagPixel{
		struct
		{
			uint8_t R;
			uint8_t G;
			uint8_t B;
		};
		struct 
		{
			uint8_t Y;
			uint8_t U;
			uint8_t V;
		};
		
 	}Pixel;
	BITMAPFILEHEADER bmpHeader;
	BITMAPFILEINFO bmpInfo;
	void rgb2yuv(Image &dst);
	void yuv2rgb(Image &dst);
	void grayscale(Image &dst);
	void resize(int w, int h, int color) {
		int sz = w * h * color;
		if (image != NULL) {
			image = (image_t*)realloc(image, sizeof(image_t) * sz);
		}
		else {
			image = (image_t*)malloc(sizeof(image_t) * sz);
		}
	}
	void inherit(Image &src) {
		status = src.status;
		memcpy(&bmpHeader, &src.bmpHeader, sizeof(BITMAPFILEHEADER));
		memcpy(&bmpInfo, &src.bmpHeader, sizeof(BITMAPFILEINFO));
	}
};

Image::Image()
	:status(rgb), image(NULL), image_color(0)
{

}

int Image::read(std::string filePath) {
	std::ifstream in;
	in.open(filePath, std::ios::in | std::ios::binary);

	//sizeof is unreliable cuz system will align elements by 4 bytes
	in.read((char *)&bmpHeader, sizeof(WORD));
	in.read((char *)&bmpHeader + 4, 12);
	in.read((char *)&bmpInfo, sizeof(BITMAPFILEINFO));
	int biBitCount = bmpInfo.biPlanes_biBitCount >> (sizeof(DWORD) * 4);
	
	in.seekg(bmpHeader.bfOffBits, in.beg);
	/*
	std::cout << "biBitCount: " << biBitCount << "\t biClrUsed: " << bmpInfo.biClrUsed << std::endl
		<< "width: " << bmpInfo.biWidth << "\t height: " << bmpInfo.biHeight << std::endl
		<< "biszie: " << bmpInfo.biSizeImage << std::endl;
	*/
	image_color = biBitCount / 8.0;
	// int imagesz = static_cast<int>(image_color * bmpInfo.biWidth * bmpInfo.biHeight);
	int imagesz = ((static_cast<int>(image_color * bmpInfo.biWidth) + 3) & ~3)* bmpInfo.biHeight;
	if (image != NULL) free(image);
	image = (image_t *)malloc(sizeof(image_t) * imagesz);
	in.read(reinterpret_cast<char *>(image), imagesz);
	in.close();
	return 0;
}

int Image::write(std::string filePath) {
	std::ofstream out;
	char dummy[8] = { 0 };
	if (image == NULL) return -1;
	int imagesz = ((static_cast<int>(image_color * bmpInfo.biWidth) + 3) & ~3)* bmpInfo.biHeight;//static_cast<int>(image_color * bmpInfo.biWidth * bmpInfo.biHeight);

	out.open(filePath, std::ios::binary | std::ios::out);
	
	out.write((char *)&bmpHeader, sizeof(WORD));
	out.write((char *)&bmpHeader + 4, 12);
	out.write((char *)&bmpInfo, sizeof(BITMAPFILEINFO));
	out.write((char *)image, imagesz);
	if (bmpInfo.biSizeImage > imagesz)
		out.write(dummy, bmpInfo.biSizeImage - imagesz);

	return 0;
}

void Image::rgb2yuv(Image &dst) {
	int max = bmpInfo.biHeight * bmpInfo.biWidth;
	int march = 3;
	if (&dst != this) {
		dst.resize(bmpInfo.biWidth, bmpInfo.biHeight, march);
		dst.inherit((*this));
	}
	for (int i=0; i < max; i+=march) {
		//Pixel* ps = reinterpret_cast<Pixel *>(image + i);
		//Pixel* pd = reinterpret_cast<Pixel *>(dst.image + i);
		Pixel* ps = (Pixel *)(image + i);
		Pixel* pd = (Pixel *)(dst.image + i);
		//std::cout << i << std::endl;
		pd->R = static_cast<uint8_t>(0.299*ps->R + 0.587*ps->G + 0.114*ps->B );
		pd->U = static_cast<uint8_t>(-0.147*ps->R - 0.289*ps->G + 0.435*ps->B);
		pd->V = static_cast<uint8_t>(0.615*ps->R - 0.515*ps->G - 0.100*ps->B );
	}
	status = yuv;
}

void Image::yuv2rgb(Image &dst) {

}
void Image::grayscale(Image &dst) {
	if (status == rgb) {
		if (&dst != this) {
			rgb2yuv(dst);
		}
	}
	int cnt = 0;/*
	for (int i = 0; i < bmpInfo.biHeight; i++) {
		Pixel* ps = (Pixel *)(dst + i);
		for (int i = 0; i < bmpInfo.biWidth; i++) {
			dst[cnt++] = dst[]
		}
	}
	*/
	

}
int main()
{
	Image im, im_yuv;
	im.read("lena508_510.bmp");
	im.rgb2yuv(im_yuv);
	im.write("yyq.bmp");
	getchar(); getchar(); getchar(); getchar();
	return 0;
}
#endif
#if 1
#define _CRT_SECURE_NO_WARNINGS
extern "C"
{
#include "bmp.h"
}
int main()
{
	Image im = imread("./lena508_510.bmp", NULL);
	Image hdr = hdr_log(NULL, im);
	imwrite("./lena508_510_hdr.bmp", hdr);
	/*

	printf("read in ./lena508_510.bmp, \ngray scale file will be saved to ./lena508_510_gray.bmp,\nwhile lumi-adjusted file will be at ./lena508_510_reconstruct.bmp\n");
	//task1: Read a color bmp
	Image im = imread("./lena508_510.bmp", NULL);
	//task 2: RGB->YUV
	Image im_yuv = rgb2yuv(NULL, im);
	//task3: Color to gray
	Image im_gray = grayScale(NULL, im_yuv);
	//task4: Rearrange gray intensity
	rearrangeGrayScale(im_gray, 0);
	//task5: Write gray bmp
	imwrite("./lena508_510_gray.bmp", im_gray);
	Image im_bin = gray2binary(NULL, im_gray);
	imwrite("./lena508_510_bin.bmp", im_bin);
	
	image_t erode_kernel[3][3] = {
		0,1,0,
		1,1,1,
		0,1,0,
	};
	Image im_bin_erd = erode(NULL, im_bin, (image_t*)erode_kernel, 3, 3);
	imwrite("./lena508_510_bin_erd.bmp", im_bin_erd);
	Image im_bin_dil = dilation(NULL, im_bin,(image_t*) erode_kernel, 3, 3);
	imwrite("./lena508_510_bin_dil.bmp", im_bin_dil);
	
	Image im_bin_closing = closing(NULL, im_bin, (image_t*)erode_kernel, 3, 3);
	imwrite("./lena508_510_bin_closing.bmp", im_bin_closing);
	Image im_bin_opening = opening(NULL, im_bin, (image_t*)erode_kernel, 3, 3);
	imwrite("./lena508_510_bin_opening.bmp", im_bin_opening);
	//task5: change luminance
	luminaceScale(im_yuv, 0.9, 0.001);
	//task6:YUV->RGB
	Image im_reconstruct = yuv2rgb(NULL, im_yuv);
	//task7: write color bmp
	imwrite("./lena508_510_reconstruct.bmp", im_reconstruct);
	*/
	printf("\n-------------THE END--------------\n");
	getchar();
	return 0;
}
#endif
#if 0
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include<stdlib.h>
typedef int ElementType;
typedef struct TreeNode *Tree;
struct TreeNode {
	ElementType Elem;
	Tree  l;
	Tree  r;
};



Tree addNode(Tree p, ElementType e) {
	Tree res = (Tree)malloc(sizeof(struct TreeNode));
	res->Elem = e;
	res->l = res->r = NULL;
	if (p == NULL) return res;
	if (p->l == NULL) { p->l = res;  return res; }
	else if (p->r == NULL) { p->r = res; return res; }
	return res;
}

void postfix(Tree t, int level) {
	if (t == NULL) return;
	postfix(t->l, level + 1);
	postfix(t->r, level + 1);
	printf("%d%s", t->Elem, level ? " ": "");
}


int main()
{
	int n;
	char cmd[100];
	ElementType e;
	scanf("%d", &n);
	n *= 2;
	Tree t = NULL;
	Tree head = NULL;
	Tree stack[100];
	int stack_ptr = 0;
	int push = 0;
	for (int i = 0; i < n; i++) {
		scanf("%s", cmd);
		switch (cmd[1])
		{
		case 'u': 
			if (push) t = stack[stack_ptr-1]; //if push previously, go deeper
			scanf("%d", &e); t = addNode(t, e); 
			if (!head) head = t; 
			stack[stack_ptr++] = t;
			push = 1;
			break;
		case 'o': push = 0;  t=stack[--stack_ptr];//t is will be parent node
		default:
			break;
		}
	}
	postfix(head, 0);
	while (1);
}



#include <stdio.h>
#include<stdlib.h>
const int Increment = 10;
typedef int elem_t;
typedef struct TreeNode *Tree;
struct TreeNode {
	elem_t id;
	int kidcnt;
	int max;
	Tree *kids;
};

typedef struct tagForest{
	Tree *ts; //trees
	int cnt;
	int max;
} ForestOrg;

typedef ForestOrg *Forest;


Tree searchT(Tree t, elem_t id) {
	if (t->id == id) return t;
	Tree tree;
	int i = 0;
	for (; i < t->kidcnt; i++) {
		tree = searchT(t->kids[i], id);
		if (tree != NULL) return tree;
	}
	return NULL;
}


Forest newF(int N)
{
	Forest f = (Forest)malloc(sizeof(ForestOrg));
	f->cnt = 0;
	f->max = N;
	f->ts = (Tree*)malloc(sizeof(Tree) * N);
	return f;
}


//search specific tree node in forest
Tree searchF(Forest f, elem_t id) {
	int i;
	for (i = 0; i < f->cnt;  i++) {
		Tree t = searchT(f->ts[i], id);
		if (t) return t;
	}
	return NULL;
}



void addT2F(Forest f, Tree t) {
	if (f->cnt == f->max) {
		f->ts = (Tree *)realloc(f->ts, (Increment + f->max) * sizeof(ForestOrg));
		f->max += Increment;
	}
		
	f->ts[f->cnt++] = t;
}

typedef struct {
	Tree a[100];
	int pivot;
	int tail;
	int size;
} Pq;
Pq pq;
void enque(Pq *pq, Tree e) {
	if (e == NULL) return;
	if (pq->tail == 100) {
		pq = 0;
	}
	pq->a[pq->tail++] = e;
	pq->size++;
}

Tree deque(Pq *pq) {
	Tree res;
	if (pq->size == 0) return NULL;
	if (pq->pivot == 100) {
		pq = 0;
	}
	res = pq->a[pq->pivot++];
	pq->size--;
	return res;
}
int isempty(Pq *pq) {
	return !(pq->size);
}
void Level_order(Tree T, void(*visit)(Tree ThisNode, int l, int isend)) {
	pq.pivot = 0;
	pq.tail = 0;
	pq.size = 0;
	enque(&pq, T);
	Tree temp;
	int level = 0;
	Tree currentLevel = T;
	while (!isempty(&pq)) {
		temp = deque(&pq);
		visit(temp, level, pq.size == 0 && pq.a[pq.tail - 1]->kidcnt == 0);
		for (int i = 0; i < temp->kidcnt; i++) {
			enque(&pq, temp->kids[i]);
		}
		if (temp == currentLevel) {
			currentLevel = pq.a[pq.tail - 1];
			level++;
		}
	}
}

Tree newT(int id) {
	Tree n = (Tree)malloc(sizeof(struct TreeNode));
	n->id = id;
	n->kidcnt = 0;
	n->max = 10;
	n->kids = (Tree *)malloc(sizeof(struct TreeNode) * n->max);
	return n;
}

void destroyT(Tree t) {
	Tree *next = t->kids;
	int cnt = t->kidcnt;
	free(t); 
	for (int i = 0; i < cnt; i++) {
		destroyT(next[i]);
	}
	free(next);
}

void addkid2T(Tree vater, Tree kid) {

	if (vater->max == vater->kidcnt) {
		vater->max += Increment;
		vater->kids = (Tree *)realloc(vater->kids, sizeof(Tree) * vater->max);
	}
	vater->kids[vater->kidcnt++] = kid;

}

void mergeT(Tree vater, Tree kid) {
	if (vater->id != kid->id) {
		addkid2T(vater, kid);
	}
	else {
		int cnt = kid->kidcnt;
		for (int i = 0; i < cnt; i++) {
			addkid2T(vater, kid->kids[i]);
			
		}
	}
}


void output(Tree t, int level, int isend) {
	static int lastLevel = 0;
	static int cnt = 0;
	if (t == NULL) {
		lastLevel = 0;
		cnt = 0;
		return;
	}
	if (lastLevel != level) {
		printf("%d ", cnt);
		lastLevel = level;
		cnt = 0;
	}
	if (t->kidcnt == 0) cnt++;
	if (isend) {
		printf("%d", cnt);
	}
	
}

//add child
void addkid2F(Forest f, Tree vater, Tree kid)
{
	if (vater == NULL)  addT2F(f, kid);
	else addkid2T(vater, kid);
}

Tree mergeF(Forest f)
{
	int cnt = f->cnt;
	int i = 0, j;
	for (; i < cnt && f->cnt > 1; i++) { // i is limit to prevent infinite loop
		for (j = 1; j < f->cnt; j++) {
			Tree vater = searchT(f->ts[0], f->ts[j]->id);
			if (vater) {
				mergeT(vater, f->ts[j]);
				f->cnt--;
				for (int k = j; k < f->cnt; k++) {
					f->ts[k] = f->ts[k + 1];
				}
			}
			else {
				vater = searchT(f->ts[j], f->ts[0]->id);
				if (vater) {
					mergeT(vater, f->ts[0]);
				}
				f->cnt--;
				for (int k = 0; k < f->cnt; k++) {
					f->ts[k] = f->ts[k + 1];
				}
			}
			
		}
	}
	return f->ts[0];
}

void emptyF(Forest f) {
	f->cnt = 0;
	f->max = 0;
	destroyT(f->ts[0]);
	f->ts[0] = NULL;
}




int main()
{
	int n, m, i, j;
	int pid, kid, kcnt;
	Forest f = newF(100);
	Tree t;
	scanf("%d%d", &n, &m);
	while (n) {
		for (j = 0; j < m; j++) {
			scanf("%d", &pid);
			t = searchF(f, pid);
			if (t == NULL) {
				t = newT(pid);
				addkid2F(f, NULL, t);
			}
			scanf("%d", &kcnt);
			for (i = 0; i < kcnt; i++) {
				scanf("%d", &kid);
				addkid2F(f, t, newT(kid));
			}
		}
		mergeF(f);
		output(NULL, 1, 1);
		if (m == 0) f->ts[0] = newT(0);
		Level_order(f->ts[0], output);
		printf("\n");
		emptyF(f);


		scanf("%d%d", &n, &m);
	}
}


typedef struct TNode *BinTree;
struct TNode {
	int Key;
	BinTree Left;
	BinTree Right;
};
typedef BinTree Tree;
typedef struct {
	Tree a[100];
	int pivot;
	int tail;
	int size;
} Pq;
Pq pq;
void enque(Pq *pq, Tree e) {
	if (e == NULL) return;
	if (pq->tail == 100) {
		pq = 0;
	}
	pq->a[pq->tail++] = e;
	pq->size++;
}

Tree deque(Pq *pq) {
	Tree res;
	if (pq->size == 0) return NULL;
	if (pq->pivot == 100) {
		pq = 0;
	}
	res = pq->a[pq->pivot++];
	pq->size--;
	return res;
}
int isempty(Pq *pq) {
	return !(pq->size);
}

void reset(Pq *pq) {
	pq->pivot = 0;
	pq->tail = 0;
	pq->size = 0;
}
int Level_order(Tree T) {
	reset(&pq);
	enque(&pq, T);
	Tree temp;
	int level = 0;
	Tree currentLevel = T;
	while (!isempty(&pq)) {
		temp = deque(&pq);
		//visit(temp, level, pq.size == 0 && pq.a[pq.tail - 1]->kidcnt == 0);
		enque(&pq, temp->Left);
		enque(&pq, temp->Right);
		if (temp == currentLevel) {
			currentLevel = pq.a[pq.tail - 1];
			level++;
		}
	}
	return ++level;
}


int preorder(Tree t, int level, void (*visit)(Tree _t))
{
	int a, b;
	if (t == NULL) return level;
	a = preorder(t->Left, level + 1, visit);
	visit(t);
	b = preorder(t->Right, level + 1, visit);
	return a > b ? a : b;
}

void record(Tree t)
{
	enque(&pq, t);
}

int CheckBST(BinTree T, int K) {
	int flag = 1; //1: bintree, -1: not bint;
	int res;
	reset(&pq);
	res = preorder(T, 0, record);
	for (int i = pq.pivot + 1; i < pq.tail; i++) {
		if (pq.a[i - 1]->Key > pq.a[i]->Key) {
			flag = -1;
			return flag * res;
		}
	}
	return pq.a[pq.tail - K]->Key;
}
BinTree newCell(int n) {
	BinTree t = (BinTree)malloc(sizeof(struct TNode));
	t->Key = n;
	t->Left = NULL;
	t->Right = NULL;
	return t;
}
BinTree BuildTree() {
	BinTree head = newCell(4);
	BinTree a, b;
	a = head->Left = newCell(3);
	b = head->Right = newCell(5);
	a->Left = newCell(1);
	a->Left->Right = newCell(2);
	a = b->Right = newCell(7);
	a->Left = newCell(6);
	a->Right = newCell(8);
	return head;
}
int main()
{
	BinTree T;
	int K, out;

	T = BuildTree();
	scanf("%d", &K);
	out = CheckBST(T, K);
	if (out < 0)
		printf("No.  Height = %d\n", -out);
	else
		printf("Yes.  Key = %d\n", out);
	while (1);
	return 0;
}

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include<stdlib.h>
typedef struct TNode *BinTree;
struct TNode {
	int Key;
	BinTree Left;
	BinTree Right;
};
typedef BinTree Tree;
BinTree newCell(int n) {
	BinTree t = (BinTree)malloc(sizeof(struct TNode));
	t->Key = n;
	t->Left = NULL;
	t->Right = NULL;
	return t;
}

Tree insert(Tree t, int n) {
	
	Tree res = newCell(n);
	Tree p = res;
	while (t) {
		p = t;
		if (n < t->Key)
			t = t->Left;
		else if (n > t->Key)
			t = t->Right;
	}
	if (n < p->Key)
		p->Left = res;
	else if (n > p->Key)
		p->Right = res;
	return res;
}
Tree read(int n) {
	int tmp;
	Tree t = NULL;
	for (int i = 0; i < n; i++) {
		scanf("%d", &tmp);
		if (t == NULL) t = insert(t, tmp);
		else insert(t, tmp);
	}
	return t;
}
int cmp(Tree t1, Tree t2)
{
	if (t1 == NULL && t2 == NULL) return 1;
	if (t1 == NULL && t2) return 0;
	if (t1 && t2 == NULL) return 0;
	if (t1->Key != t2->Key) return 0;
	return cmp(t1->Left, t2->Left) && cmp(t1->Right, t2->Right);
}
int main()
{
	int n, l;
	scanf("%d", &n);
	while (n) {
		scanf("%d", &l);
		Tree t = read(n);
		for (int i = 0; i < l; i++) {
			Tree tt = read(n);
			if (cmp(t, tt))
				printf("Yes\n");
			else
				printf("No\n");
		}
		scanf("%d", &n);
	}
}
#endif
#if 0
#include <stdio.h>
#include <stdlib.h>
typedef int elem_t;
void exch(elem_t *e, int a, int b) {
	elem_t tmp;
	tmp = e[a];
	e[a] = e[b];
	e[b] = tmp;
}
void m_qsort(elem_t *e, int left, int right)
{
	if (left >= right) return;
	int mid = (left + right) / 2;
	elem_t id = e[mid], tmp;
	exch(e, left, mid);
	int l = left;
	int r = right;
	while (l < r) {
		while (e[r] >= id && l < r)
			r--;
		while (e[l] <= id && l < r)
			l++;
		if(l < r)
			exch(e, l, r);
	}
	exch(e, left, l);
	m_qsort(e, left, l - 1);
	m_qsort(e, l + 1, right);
}
typedef struct TNode *BinTree;
struct TNode {
	elem_t Key;
	BinTree Left;
	BinTree Right;
};
typedef BinTree Tree;
BinTree newCell(int n) {
	BinTree t = (BinTree)malloc(sizeof(struct TNode));
	t->Key = n;
	t->Left = NULL;
	t->Right = NULL;
	return t;
}

typedef struct {
	Tree a[10000];
	int pivot;
	int tail;
	int size;
} Pq;
Pq pq;
void enque(Pq *pq, Tree e) {
	if (e == NULL) return;
	if (pq->tail == 10000) {
		pq = 0;
	}
	pq->a[pq->tail++] = e;
	pq->size++;
}

Tree deque(Pq *pq) {
	Tree res;
	if (pq->size == 0) return NULL;
	if (pq->pivot == 10000) {
		pq = 0;
	}
	res = pq->a[pq->pivot++];
	pq->size--;
	return res;
}
int isempty(Pq *pq) {
	return !(pq->size);
}
void Level_order(Tree T, void(*visit)(Tree ThisNode, int l, int isend)) {
	pq.pivot = 0;
	pq.tail = 0;
	pq.size = 0;
	enque(&pq, T);
	Tree temp;
	int level = 0;
	Tree currentLevel = T;
	while (!isempty(&pq)) {
		temp = deque(&pq);
		visit(temp, level, pq.size == 0 && pq.a[pq.tail - 1]->Right == NULL && pq.a[pq.tail - 1]->Left == NULL);
		enque(&pq, temp->Left);
		enque(&pq, temp->Right);
		if (temp == currentLevel) {
			currentLevel = pq.a[pq.tail - 1];
			level++;
		}
	}
}
void output(Tree t, int level, int isend){
	printf("%d%s", t->Key, isend ? "\n" : " ");
}

#define LEFT 1
#define RIGHT -1
Tree build(elem_t *arr, int n)
{
	Tree newT = NULL;
	if (n <= 0) return NULL;
	int level = 1;
	int cnt = n - 1;
	int flag = LEFT;
	int left = 0;
	int right = 0;
	int mid = 0;
	while (left + right < cnt) {
		if (flag == LEFT) {
			left += level;
		}
		else {
			right += level;
			level *= 2;
		}
		flag = -flag;
	}
	mid = (flag == LEFT) ? left : (cnt - right);
	newT = newCell(arr[mid]);
	newT->Left = build(arr, mid);
	newT->Right = build(arr + mid + 1, n - mid - 1);
	return newT;
}

int main()
{
	int i,n;
	elem_t arr[10000];
	scanf("%d", &n);
	for (i = 0; i < n; i++) {
		scanf("%d", arr + i);
	}
	m_qsort(arr, 0, n - 1);
	for (i = 0; i < n; i++) {
		printf("%d ", *(arr + i));
	}
	printf("\n");
	Tree t = build(arr, n);
	Level_order(t, output);
	printf("\n");
	while (1)
	{

	}
	return 0;
}
#endif