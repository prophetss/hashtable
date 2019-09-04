# hashtable
Hash function based on [xxhash](https://github.com/Cyan4973/xxHash)

在gcc和VC下C/C++都已运行通过，支持创建、查找、添加、删除、获取所有元素、重哈希和自定义比较函数。有引用（key拷贝数据value拷贝指针）拷贝（key拷贝数据value拷贝数据）两种模式可选。自动扩缩容，负载均衡（内部存在两张表，不会扩容rehash导致某单次速度过慢）

简单实测电脑配置AMD Ryzen 7 3700X 3.60 GHz Linux64位，test_hash.c内demo，1.0负载因子下，连续插入100万条随机key-value（各16字节）速度400万次/s，连续查询100万条随机key速度1000万次/s。
