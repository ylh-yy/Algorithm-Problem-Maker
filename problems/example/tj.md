# 草地上的狗和袋鼠题解

## 1. 计算可选的狗数量

设狗的数量为 $d$，袋鼠的数量为 $k$。两者都是非负整数，且 $4d+2k=n$。

当 $n$ 为奇数时，等式左边始终为偶数，答案为 $0$。

当 $n$ 为偶数时，最多可以有 $\lfloor n/4\rfloor$ 只狗。对每个整数 $d$（$0\le d\le\lfloor n/4\rfloor$），剩余的 $n-4d$ 是非负偶数，因此恰好对应一种袋鼠数量 $k=(n-4d)/2$。不同的 $d$ 对应不同的搭配，所有搭配也都在这个范围内。算上 $d=0$，一共有 $\lfloor n/4\rfloor+1$ 种。

在 C++ 中，`n` 是正整数，`n / 4` 会舍去小数部分。也可以从 $0$ 开始逐个枚举狗的数量，判断剩余腿数是否为非负偶数。

## 2. 复杂度

每个测试用例用时 $O(1)$，处理全部用例用时 $O(t)$，额外空间为 $O(1)$。

## 3. 参考代码

```cpp
#include <iostream>
using namespace std;

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int t;
    cin >> t;
    for (int i = 0; i < t; ++i) {
        int n;
        cin >> n;
        if (n % 2 != 0) {
            cout << 0 << '\n';
        } else {
            cout << n / 4 + 1 << '\n';
        }
    }
    return 0;
}
```