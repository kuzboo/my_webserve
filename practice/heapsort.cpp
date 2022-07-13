#include<vector>
#include<iostream>
using namespace std;


/*建堆*/
//下标为i的父节点(i-1)/2
//下标为i的左孩子i*2+1
//下标为i的右孩子i*2+2

void heapget(vector<int> &nums,int n,int i)
{
    int max = i;
    int lson = i * 2 + 1, rson = i * 2 + 2;
    if(lson<n && nums[lson]>nums[max])
        max = lson;
    if(rson<n && nums[rson]>nums[max])
        max = rson;
    if(max!=i)
    {
        swap(nums[max], nums[i]);
        heapget(nums, n, max);
    }
}

/*排序*/
//从最后一个节点的父节点开始建堆
//倒叙遍历 每次交换最后一个与第一个
void heapsort(vector<int> &nums,int n)
{
    int i;
    //建堆从最后一个节点的父节点
    for (int i = n / 2 - 1;i>=0;--i)
    {
        heapget(nums, n, i);
    }
    //交换
    for (int i = n - 1; i >= 0;--i)
    {
        swap(nums[0], nums[i]);
        heapget(nums, i, 0);
    }
}