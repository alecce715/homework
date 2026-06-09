<?php
// 设置缓存时间为 30 秒
header('Cache-Control: max-age=30');
$timestamp = time();
?>
<!DOCTYPE html>
<html>
<head>
    <title>Cache Demo</title>
</head>
<body>
    <h1>缓存演示</h1>
    <p>当前时间戳：<?php echo $timestamp; ?></p>
    <p>格式化时间：<?php echo date('Y-m-d H:i:s', $timestamp); ?></p>
    
    <h2>测试说明：</h2>
    <ul>
        <li>第一次访问时，服务器返回新内容</li>
        <li>30 秒内刷新，浏览器会使用缓存（Size 列显示 memory cache 或 disk cache）</li>
        <li>30 秒后刷新，时间戳会更新</li>
    </ul>
</body>
</html>