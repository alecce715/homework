<?php
// 设置会话级 Cookie（不设置过期时间）
setcookie('session_token', 'abc123');
?>
<!DOCTYPE html>
<html>
<head>
    <title>Set Cookie</title>
</head>
<body>
    <h1>Cookie 设置成功！</h1>
    <p>已设置会话级 Cookie：session_token = abc123</p>
    <p>请打开 F12 → Storage/Application → Cookies 查看</p>
</body>
</html>