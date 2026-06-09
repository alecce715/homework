<?php
$user_agent = isset($_SERVER['HTTP_USER_AGENT']) ? $_SERVER['HTTP_USER_AGENT'] : '未知';
$accept_language = isset($_SERVER['HTTP_ACCEPT_LANGUAGE']) ? $_SERVER['HTTP_ACCEPT_LANGUAGE'] : '未知';
$accept_encoding = isset($_SERVER['HTTP_ACCEPT_ENCODING']) ? $_SERVER['HTTP_ACCEPT_ENCODING'] : '未知';

// 检测是否为 curl
if (strpos(strtolower($user_agent), 'curl') !== false) {
    $special_message = '<p style="color: orange; font-weight: bold;">🤖 机器人侦探，欢迎使用命令行</p>';
} else {
    $special_message = '';
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>User Agent Info</title>
</head>
<body>
    <h1>浏览器信息检测</h1>
    
    <h2>你的浏览器是：</h2>
    <p><?php echo htmlspecialchars($user_agent); ?></p>
    
    <?php echo $special_message; ?>
    
    <h2>语言偏好 (Accept-Language)：</h2>
    <p><?php echo htmlspecialchars($accept_language); ?></p>
    
    <h2>压缩方式 (Accept-Encoding)：</h2>
    <p><?php echo htmlspecialchars($accept_encoding); ?></p>
    
    <h2>测试方法：</h2>
    <ul>
        <li>使用不同浏览器访问，查看 User-Agent 变化</li>
        <li>使用开发者工具模拟手机浏览器</li>
        <li>使用 curl 命令：<code>curl http://localhost/ua.php</code></li>
    </ul>
</body>
</html>