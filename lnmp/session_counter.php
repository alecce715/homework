<?php
session_start();

if (!isset($_SESSION['visit_count'])) {
    $_SESSION['visit_count'] = 1;
    $_SESSION['last_visit'] = date('Y-m-d H:i:s');
    echo "<h1>欢迎首次来访！</h1>";
} else {
    $_SESSION['visit_count']++;
    $last_visit = $_SESSION['last_visit'];
    $_SESSION['last_visit'] = date('Y-m-d H:i:s');
    echo "<h1>这是您第 {$_SESSION['visit_count']} 次访问</h1>";
    echo "<p>上次访问时间为：{$last_visit}</p>";
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>Session Counter</title>
</head>
<body>
    <p>刷新页面查看访问次数变化</p>
    <p>关闭浏览器再打开，计数器会重置（会话级 Session 默认行为）</p>
</body>
</html>