// ROUTE.cpp : 定义控制台应用程序的入口点。
//

/*
待完成的部分：
1.如果两根线连续重叠的点超过一个，则需要智能分析出来，则不换线经过这些重叠的点
2.如果要从起点找到终点的时候，可以只从线的交点考虑，起点所在的线的交点到达终点所在线的交点找出所有的路径，
  然后再填充中间的线上所经过的点
*/

#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stack.h"

#define LINE_TYPE_I 0
#define LINE_TYPE_O 1

#define ONE_POINT_MAX_CROSS_N  4
#define ONE_LINE_MAX_POINT_N   64
#define ONE_LINE_MAX_CROSS_N   8  
  
#define POINT_MAX_N 4096
#define LINE_MAX_N  1024
#define POINT_AT_HEAD 1
#define POINT_AT_TAIL 0

typedef struct POINT_STRU POINT;
typedef struct LINE_STRU  LINE;

struct POINT_STRU
{
	int id;
	int type;//0:common 1:cross
	POINT *next[ONE_POINT_MAX_CROSS_N];
	POINT *prev[ONE_POINT_MAX_CROSS_N];
	int lineId[ONE_POINT_MAX_CROSS_N];
	int posInLine[ONE_POINT_MAX_CROSS_N];//它在线上属于第几个点
	int crossNum;//经过该点的个数
	int routeFartherArr[ONE_POINT_MAX_CROSS_N];
	int arrLen;
};

struct LINE_STRU
{
	int id;
	int type;
	POINT *first;
	int nextPosition[ONE_LINE_MAX_POINT_N];
	int pointN;
	int direc;
	POINT *pCrossPoint[ONE_LINE_MAX_CROSS_N];
	int crossNum;
	
	struct
	{
		LINE *line;
		int pCrossPoint[16];
	}friendLine[16];
};

POINT *g_crossPoint[POINT_MAX_N];
int crossPointNum = 0;

POINT g_point[POINT_MAX_N];
static int pointNum = 0;

LINE g_line[LINE_MAX_N];
static int lineNum = 0;
void buildLinePrev(LINE *l,int index);

void *stackHandle;


void printArr(int *arr,int len)
{
	int i;
	for(i = 0;i < len;i++)
	{
		printf("%d ",arr[i]);
	}
	printf("\n");
}

int printLineCrossPoint(LINE *l,int index)
{
	LINE *line;
	int i;
	
	line = &l[index];
	printf("printLineCrossPoint index:%d:\n",index);

	for(i = 0;i < line->crossNum;i++)
	{
		printf("%d ",line->pCrossPoint[i]->id);
	}
	printf("\n");

	return 0;
}

int getLineIndexById(int id)
{
	int i;
	for(i=0;i<lineNum;i++)
	{
		if(g_line[i].id == id)
			return i;
	}
	
	return -1;
}


int getPointIndexById(int id)
{
	int i;
	for(i=0;i<pointNum;i++)
	{
		if(g_point[i].id == id)
			return i;
	}
	
	return -1;
}

void fflushCrossInfo(POINT *p)
{
	LINE *line;
	
	if(p->crossNum == 1)
	{
		line = &g_line[getLineIndexById(p->lineId[0])];
		line->pCrossPoint[line->crossNum++] = p;
	}
	if(p->crossNum >= 1)
	{
		g_line[lineNum].pCrossPoint[g_line[lineNum].crossNum++] = p;
	}
}

int isPointOnLine(POINT *point,int lineId)
{
	int i;
	
	for(i=0;i<point->crossNum;i++)
	{
		if(lineId == point->lineId[i])
			return 1;
	}
	return 0;
}

int getPointPosOnLine(POINT *point,int lineId)
{
	int lineIndex;
	LINE *line;
	int i;
	
	if(point == NULL)
	{
		printf("getPointIndexOnLine error 0!\n");
		return -1;
	}
	lineIndex = getLineIndexById(lineId);
	line = &g_line[lineIndex];

	for(i=0;i<point->crossNum;i++)
	{
		if(point->lineId[i] == lineId)
		{
			return point->posInLine[i];
		}
	}
	
	printf("getPointIndexOnLine error 1!\n");
	return -1;
}


/*返回下一个点的ID*/
POINT *moveOnLine(POINT *point,int lineId,int direction)
{
	int lineIndex;
	LINE *line;
	int nextPointId;
	int tmp; 
	int posOnLine;
	
	lineIndex = getLineIndexById(lineId);
	line = &g_line[lineIndex];

	posOnLine = getPointPosOnLine(point,lineId);
	if(posOnLine < 0)
	{
		printf("getPointIndexOnLine error!\n");
		return NULL;
	}
	tmp = line->nextPosition[posOnLine];
	if(direction == POINT_AT_HEAD)
	{
		point = point->prev[tmp];
	}
	else
	{
		point = point->next[tmp];
	}

	return point;	
}

/*
用另一种办法递归
写line从某一点向前走和向后走的函数
recursionRoute:遍历出所有的点
*/
void priArr(int *arr,int len)
{
	int i;
	printf("<");
	for(i = 0;i < len;i++)
	{
		printf("%d ",arr[i]);
	}
	printf(">\n");
}
#if 1
int isExistInArr(int *arr,int len,int val)
{
	int i;
	
	for(i = len - 1;i >= 0;i--)
	{
		if(arr[i] == val)
			return (i+1);
	}
	return 0;
}
#else
int isExistInArr(int *arr,int arrLen,int *father,int len,int val)
{
	int i;
	int index;
	
	for(i = 0;i < len;i++)
	{
		index = father[i];
		if(index < 0 || index >= arrLen)
			return 0;
		if(val == arr[index])
			return (i+1);
	}
	
	return 0;
}
#endif

int delInArr(int *arr,int *len,int FromVal)
{
	int i;
	
	for(i = (*len) - 1;i >= 0;i--)
	{
		if(arr[i] == FromVal)
		{
			*len = i + 1;
			return 1;
		}
	}
	return -1;
}
void GetAllRoute(POINT *start,int *routeNum)//高效通用
{
	POINT *p,*pTmp;
	int indexFarther;
	int i,j,indexInLine,tmp;
	LINE *line;
	int arr[1024];
	int len = 0;
	int arrStack[16];
	int flag = 0;
	int fatherIndex,fatherId1;
	
	printf("GetAllRoute:\n");
	(*routeNum) = 0;
	//找出开始节点所有的子节点入栈
	arr[len++] = start->id;
	for(i = 0;i < start->crossNum;i++)
	{
		pTmp = start->next[i];
		if(pTmp != NULL && pTmp != start)
		{
			STACK_push(stackHandle,&pTmp);
			pTmp->routeFartherArr[pTmp->arrLen++] = 0;
			//pTmp->arrTmp[pTmp->arrTmpLen++] = start->id;
		}
		pTmp = start->prev[i];
		if(pTmp != NULL && pTmp != start) 
		{
			STACK_push(stackHandle,&pTmp);
			pTmp->routeFartherArr[pTmp->arrLen++] = 0;
			//pTmp->arrTmp[pTmp->arrTmpLen++] = start->id;
		}
	}

	while(!STACK_empty(stackHandle))
	{
		//0.出一个节点的栈
		STACK_pop(stackHandle,&p);
		//1.如果是叶子节点则放入数组并打印出数组，跳到步骤0
		if(p->crossNum == 1 && (p->next[0] == NULL || p->prev[0] == NULL))
		{
			fatherIndex = p->routeFartherArr[--p->arrLen];
			len = fatherIndex + 1;
			arr[len++] = p->id;
			priArr(arr,len);
			(*routeNum)++;
			continue;
		}
		fatherIndex = p->routeFartherArr[--p->arrLen];
		len = fatherIndex + 1;
		arr[len++] = p->id;
		flag = 0;		
		for(i = 0;i < p->crossNum;i++)
		{
			pTmp = p->next[i];
			if(pTmp != NULL)
			{
				if(pTmp->id != arr[fatherIndex])
				{
					if(isExistInArr(arr,len,pTmp->id))
					{
						flag = 1;
					}
					else
					{
						pTmp->routeFartherArr[pTmp->arrLen++] = len - 1;
						STACK_push(stackHandle,&pTmp);		
					}
				}
				
			}
			pTmp = p->prev[i];
			if(pTmp != NULL)
			{
				if(pTmp->id != arr[fatherIndex])
				{
					if(isExistInArr(arr,len,pTmp->id))
					{	
						flag = 1;	
					
					}
					else
					{
						pTmp->routeFartherArr[pTmp->arrLen++] = len - 1;
						STACK_push(stackHandle,&pTmp);
					}
				}
				
			}
		}
		if(flag)
		{
			priArr(arr,len);
			(*routeNum)++;
		}
	}
}
void GetMinLineNumRoute(POINT *start,POINT *end)
{
	
	return ;
} 

void GetShortestRoute(POINT *start,POINT *end)//高效通用
{
	POINT *p,*pTmp;
	int indexFarther;
	int i,j,indexInLine,tmp;
	LINE *line;
	int arr[1024];
	int len = 0;
	int arrStack[16];
	int flag = 0;
	int fatherIndex,fatherId1;
	int stepNum = 2^31;
	
	printf("From %d to %d:\n",start->id,end->id);
	//找出开始节点所有的子节点入栈
	arr[len++] = start->id;
	for(i = 0;i < start->crossNum;i++)
	{
		pTmp = start->next[i];
		if(pTmp != NULL)
		{
			if(pTmp->id == end->id)
			{
				arr[len++] = pTmp->id;
				priArr(arr,len);
				stepNum = len;
			}
			else
			{
				STACK_push(stackHandle,&pTmp);
				pTmp->routeFartherArr[pTmp->arrLen++] = 0;
			}
		}
		pTmp = start->prev[i];
		if(pTmp != NULL) 
		{
			if(pTmp->id == end->id)
			{
				arr[len++] = pTmp->id;
				priArr(arr,len);
				stepNum = len;
			}
			else
			{
				STACK_push(stackHandle,&pTmp);
				pTmp->routeFartherArr[pTmp->arrLen++] = 0;
			}
		}
	}

	while(!STACK_empty(stackHandle))
	{
		//0.出一个节点的栈
		STACK_pop(stackHandle,&p);
		//1.如果是叶子节点则放入数组并打印出数组，跳到步骤0
		if(p->crossNum == 1 && (p->next[0] == NULL || p->prev[0] == NULL))
		{
			fatherIndex = p->routeFartherArr[--p->arrLen];
			len = fatherIndex + 1;
			arr[len++] = p->id;
		//	priArr(arr,len);
			continue;
		}
		fatherIndex = p->routeFartherArr[--p->arrLen];
		len = fatherIndex + 1;
		arr[len++] = p->id;
		
		if(len >= stepNum)
			continue;
		flag = 0;
		for(i = 0;i < p->crossNum;i++)
		{
			pTmp = p->next[i];
			if(pTmp != NULL)
			{
				if(pTmp->id != arr[fatherIndex])
				{
					if(!isExistInArr(arr,len,pTmp->id))
					{
						pTmp->routeFartherArr[pTmp->arrLen++] = len - 1;
						if(pTmp->id == end->id)
						{
							flag = 1;
							stepNum = len;
							arr[len] = pTmp->id;
						}
						else
						{
							STACK_push(stackHandle,&pTmp);
						}
					}
				}
			}
			pTmp = p->prev[i];
			if(pTmp != NULL)
			{
				if(pTmp->id != arr[fatherIndex])
				{
					if(!isExistInArr(arr,len,pTmp->id))
					{
						if(pTmp->id == end->id)
						{
							flag = 1;
							stepNum = len;
							arr[len] = pTmp->id;
						}
						else
						{
							pTmp->routeFartherArr[pTmp->arrLen++] = len - 1;
							STACK_push(stackHandle,&pTmp);
						}
					}
				}	
			}
		}
		if(flag)
		{
			len++;
			priArr(arr,len);
		}
	}
}

void addLine(int id,int *line,int len)
{
	int i;
	int index;
	POINT *last;
	POINT *curr;
	int crossNum;
	int flag = 0;
	int *position;
	int isFirstExist = 0;
	LINE *lineTmp;
	
	position = &g_line[lineNum].pointN;
	/*判断点的类型*/
	if(line[0] == line[len-1])
	{
		g_line[lineNum].type = LINE_TYPE_O;
	}
	else
	{
		g_line[lineNum].type = LINE_TYPE_I;
	}
	
	g_line[lineNum].id = id;
	
	index = getPointIndexById(line[0]);
	if(index < 0)
	{
		isFirstExist = 0;
		g_line[lineNum].first = &g_point[pointNum];
	}
	else
	{
		isFirstExist = 1;
		g_line[lineNum].first = &g_point[index];
	}
	
	for(i=0;i<len;i++)
	{
		flag = 0;
		index = getPointIndexById(line[i]);
		if(index < 0)
		{
			/*不存在*/
			curr = &g_point[pointNum];
			curr->id = line[i];  
			flag = 1;
		}
		else
		{
			curr = &g_point[index];
		}
		
		if(i == 0)
		{
			if(g_line[lineNum].type == LINE_TYPE_I)
			{
				curr->prev[curr->crossNum] = NULL;
			}
		}
		else if(i>0 && i< len-1)
		{
			last->next[last->crossNum] = curr;
			curr->prev[curr->crossNum] = last;
			last->posInLine[last->crossNum] = i - 1;
			last->lineId[last->crossNum] = id;
			fflushCrossInfo(last);
			g_line[lineNum].nextPosition[(*position)++] = last->crossNum++;
		
		}
		else
		{
			if(g_line[lineNum].type == LINE_TYPE_O)
			{
				crossNum = g_line[lineNum].first->crossNum - 1;	
				last->next[last->crossNum] = g_line[lineNum].first;
				g_line[lineNum].first->prev[crossNum] = last;
				if(flag)
				{
					pointNum++;
				}
				break;
			}
			else
			{
				curr->prev[curr->crossNum] = last;
				last->next[last->crossNum] = curr;

				if(g_line[lineNum].type == LINE_TYPE_I)
				{
					curr->next[curr->crossNum] = NULL;
				}
				last->posInLine[last->crossNum] = i - 1;
				last->lineId[last->crossNum] = id;
				fflushCrossInfo(last);
				g_line[lineNum].nextPosition[(*position)++] = last->crossNum++;
				
			}
		}
		if(flag)
		{
			pointNum++;
		}
		last = curr;
	}
	last->posInLine[last->crossNum] = i - 1;
	last->lineId[last->crossNum] = id;
	fflushCrossInfo(last);
	g_line[lineNum].nextPosition[(*position)++] = last->crossNum++;	
	lineNum++;
}

void printLinePointExt(LINE *l,int index)
{
	POINT *p;
	LINE *line;
	int tmp,j;
	int newIndex;
	
	line = &l[index];
	p = line->first;

	if(line->type == LINE_TYPE_I)
	{	
		j = 0;
		while(p != NULL)
		{
			tmp = line->nextPosition[j];
			if(p->next[tmp] == NULL)
			{
				break;
			}
			p = p->next[tmp]; 
			j++;
		}
		j = line->pointN - 1;	
		while(p != NULL)
		{
			tmp = line->nextPosition[j]; 
			printf("index:%d %d\n",p->posInLine[tmp],p->id);
			p = p->prev[tmp];
			j--;
		}
	}
	else
	{
		j = 0;
		tmp = line->nextPosition[j];
		p = p->prev[tmp];
		j = line->pointN - 1;	
		
		while(p != line->first)
		{
			tmp = line->nextPosition[j]; 
			printf("index:%d %d\n",p->posInLine[tmp],p->id);
			p = p->prev[tmp];
			j--;
		}

		printf("index:%d %d\n",p->posInLine[line->nextPosition[0]],p->id);
	}	
}

void printLinePointIndex(LINE *l,int index)
{
	LINE *line;
	line = &l[index];
	int i;
	
	printf("printLinePointIndex:\n");
	for(i = 0;i<line->pointN;i++)
	{
		printf("%d ",line->nextPosition[i]);
	}
	printf("\n");
}

void printLinePointByMove(LINE *l,int index)
{
	LINE *line;
	POINT *point;
	int pointNum;
	
	line = &l[index];
	point = line->first;
	pointNum = line->pointN;
	printf("printLinePointByMove:\n");
	printf("%d ",point->id);
	pointNum--;
	
	while(pointNum--)
	{
		point = moveOnLine(point,line->id,POINT_AT_TAIL);
		if(point != NULL)
		{
			printf("%d ",point->id);
		}
	}
	printf("\n");
}

void printLinePoint(LINE *l,int index)
{
	POINT *p;
	LINE *line;
	int tmp,j = 0;
	line = &l[index];
	
	p = line->first;
	printf("-->line:%d\n",line->id);
	
	if(line->type == LINE_TYPE_I)
	{
		while(p != NULL)
		{
			tmp = line->nextPosition[j];
			printf("index:%d %d\n",p->posInLine[tmp],p->id);
			
			p = p->next[tmp];
			j++;
		}
	}
	else
	{
		tmp = line->nextPosition[j];
		printf("index:%d %d\n",p->posInLine[tmp],p->id);
		p = p->next[tmp];
		j++;
		while(p != line->first)
		{
			tmp = line->nextPosition[j];
			printf("index:%d %d\n",p->posInLine[tmp],p->id);
			p = p->next[tmp];
			j++;
		}
		
	}
	printf("\n");
}

int _tmain(int argc, _TCHAR* argv[])
{

	int num,i,index;
	POINT *start,*end;
	int line1[] = {15,1,18,20,15};
	int line2[] = {15,8,3543,4,25};
	int line3[] = {67,1,565,111};
	int line4[] = {23,18,565,24,23};
	int line5[] = {222,9,565};
	int line6[] = {18,4,25};
	
	int len1 = sizeof(line1)/sizeof(int);
	int len2 = sizeof(line2)/sizeof(int);
	int len3 = sizeof(line3)/sizeof(int);
	int len4 = sizeof(line4)/sizeof(int);
	int len5 = sizeof(line5)/sizeof(int);
	int len6 = sizeof(line6)/sizeof(int);
	
 	STACK_init(&stackHandle,1024,sizeof(POINT *));
	memset(g_line,'\0',sizeof(g_line));
	memset(g_point,'\0',sizeof(g_point));
	
	addLine(1,line1,len1);
	addLine(2,line2,len2);
	addLine(3,line3,len3);
	addLine(4,line4,len4);
	addLine(5,line5,len5);
	addLine(6,line6,len6);
	printLinePointIndex(g_line,0);
	printLinePointIndex(g_line,1);
	printLinePointIndex(g_line,2);
	printLinePointIndex(g_line,4);
	
	printLinePoint(g_line,0);
	printLinePoint(g_line,1);
	printLinePoint(g_line,2);
	printLinePoint(g_line,3);
	
	printLineCrossPoint(g_line,0);
	printLineCrossPoint(g_line,1);
	printLineCrossPoint(g_line,2);
	printLineCrossPoint(g_line,3);

	printLinePointExt(g_line,0);
	printLinePointExt(g_line,1);
	printLinePointExt(g_line,2);
	printLinePointExt(g_line,3);
	{
		POINT *point;
		POINT *pointCurr;
		
		point = &g_point[0];
		pointCurr = point;
		for(i = 0;i < pointNum;i++)
		{
			//printf("==============%d=================\n",i);
		//	point = &g_point[getPointIndexById(565)];
			point = &g_point[i];
			//GetAllRoute(point,&num);
		//	printf("------------------------>num:%d\n",num);
		}
	}
#define POINT_STRAT  25 
#define POINT_END    111

	if((index = getPointIndexById(POINT_STRAT)) < 0)
	{
		printf("No this point!!!\n");
		return -1;
	}
	start = &g_point[index];
	
	if((index = getPointIndexById(POINT_END)) < 0)
	{
		printf("No this point!!!\n");
		return -1;
	}
	end = &g_point[index];
	
	GetShortestRoute(start,end);
	GetMinLineNumRoute(start,end);
	system("pause");
}




