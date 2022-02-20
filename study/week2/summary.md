# 第二周总结：
将项目第一部分完成，并学习c++。在其他时间看了算法和面经，但是感觉不足的东西太多了，时间不够。c++部分学了很多以前不知道的东西。然后对于项目来说，太过于吃力了，跟做完一部分还是一头雾水，不知道自己在干嘛。
## 智能指针
### auto_ptr
当我们用new申请一部分内存，用一个指针来保存起来。而在使用之后需要对内存进行释放。而有时会因为忘记释放而造成内存泄漏。在大型的项目中，这种泄露又很难查找，而智能指针就是为了解决内存泄漏而产生的。

使用时，需要包含头文件：memory。

智能指针其实是一个类，可以通过将普通指针作为参数传入智能指针的构造函数实现绑定。只不过通过运算符重载让它“假装”是一个指针，也可以进行解引用等操作。既然智能指针是一个类，对象都存在于栈上，那么创建出来的对象在出作用域的时候(函数或者程序结束)会自己消亡，所以在这个类中的析构函数中写上delete就可以完成智能的内存回收。
支持拷贝构造，支持赋值拷贝，支持operator->/operator*解引用，支持指针变量重置，保证指针持有者唯一

auto_ptr <double> pd;

double *p_reg = new double;

pd = p_reg; // 不允许

pd = auto_ptr <double> (p_reg); //允许

auto_ptr <double> panto =p_reg; //不允许

auto_ptr <double> pauto (p_reg); //允许

而auto_ptr存在大量弊端

数组保存std::auto_ptr实例

std::vector<std::auto_ptr<People>> peoples;
// 这里实例化多个people并保存到数组中
...
std::auto_ptr<People> one = peoples[5];
...
std::cout << peoples[5]->get_name() << std::endl; 
上面程序如果将std::auto_tr类型替换为原始指针，就不会有问题。但是这里却会导致程序报段错误崩溃！问题就出在：

std::auto_ptr<People> one = peoples[5];
这行代码会将peoples[5]中的指针所有权转移了！即该变量中的指针已经为null了。后续对解引用是不正确的。

函数传参
void do_somthing(std::auto_ptr<People> people){
    // 该函数内不对people变量执行各种隐式/显示的所有权转移和释放
    ...
}

std::auto_ptr<People> people(new People("jony"));
do_something(people);
...

std::cout << people->get_name() << std::endl; 
这里对auto_ptr的使用与原始指针的使用完全相同。但是该代码还是会崩溃！问题就在于执行do_something()函数时，传递参数导致原变量的指针所有权转移了，即people变量实际已经变为null了！

原因在于std::auto_ptr支持拷贝构造，为了确保指针所有者唯一，这里转移了所有权

如今auto_ptr已经被废除了，已经被unique_ptr取代
## unique_ptr

智能指针实际上是一个对象，在对象的外面包围了一个拥有该对象的普通指针。这个包围的常规指针称为裸指针。

智能指针类可以通过它所指向的对象类型设置形参。例如，unique_ptr<int> 就是一个指向 int 的指针；而 unique_ptr<double> 就是一个指向 double 的指针。以下代码显示了如何创建独占指针：
unique_ptr<int> uptr1(new int);
unique_ptr<double> uptr2(new double);

当然，也可以先定义一个未初始化的指针，然后再给它赋值：
unique_ptr<int> uptr3;
uptr3 = unique_ptr<int> (new int);

为了避免内存泄漏，通过智能指针管理的对象应该没有其他的引用指向它们。换句话说，指向动态分配存储的指针应该立即传递给智能指针构造函数，而不能先将它赋值给指针变量。

例如，应该避免按以下方式编写代码：
int *p = new int;
unique_ptr<int> uptr(p);

智能指针不支持指针的算术运算，所以下面的语句将导致编译器错误：
uptr1 ++;
uptr1 = uptr1 + 2;

但是，智能指针通过运算符重载支持常用指针运算符 * 和 ->。以下代码将解引用一个独占指针，以给动态分配内存位置赋值，递增该位置的值，然后打印结果：
unique_ptr<int> uptr(new int);
*uptr = 12;
*uptr = *uptr + 1;
cout << *uptr << endl;

不能使用其他 unique_ptr 对象的值来初始化一个 unique_ptr。同样，也不能将一个 unique_ptr 对象赋值给另外一个。这是因为，这样的操作将导致两个独占指针共享相同对象的所有权，所以，以下语句都将出现编译时错误：
unique_ptr<int> uptr1(new int);
unique_ptr<int> uptr2 = uptr1; // 非法初始化
unique_ptr<int> uptr3; // 正确
uptr3 = uptr1; // 非法赋值

C++ 提供了一个 move() 库函数，可用于将对象的所有权从一个独占指针转移到另外一个独占指针：
unique_ptr<int> uptr1(new int);
*uptr1 = 15;
unique_ptr<int> uptr3; // 正确
uptr3 = move (uptr1) ; // 将所有权从 uptr1 转移到 uptr3
cout << *uptr3 << endl; // 打印 15

假设存在以下转移语句：
U = move(V);

那么，当执行该语句时，会发生两件事情。首先，当前 U 所拥有的任何对象都将被删除；其次，指针 V 放弃了原有的对象所有权，被置为空，而 U 则获得转移的所有权，继续控制之前由 V 所拥有的对象。

# share_ptr weak_ptr
hared_ptr和weak_ptr 基础概念
shared_ptr与weak_ptr智能指针均是C++ RAII的一种应用，可用于动态资源管理
shared_ptr基于“引用计数”模型实现，多个shared_ptr可指向同一个动态对象，并维护了一个共享的引用计数器，记录了引用同一对象的shared_ptr实例的数量。当最后一个指向动态对象的shared_ptr销毁时，会自动销毁其所指对象(通过delete操作符)。
shared_ptr的默认能力是管理动态内存，但支持自定义的Deleter以实现个性化的资源释放动作。
weak_ptr用于解决“引用计数”模型循环依赖问题，weak_ptr指向一个对象，并不增减该对象的引用计数器

构造方法: 
1.通过make_shared函数构造 
auto s_s = make_shared(“hello”);

2.通过原生指针构造 
int* pNode = new int(5); 
shared_ptr s_int(pNode); 
//获取原生指针 
int* pOrg = s_int.get();

3.通过赋值函数构造shared_ptr

4.重载的operator->, operator *，以及其他辅助操作如unique()、use_count(), get()等成员方法。
