<?php
// 获取请求的状态码参数
$code = isset($_GET['code']) ? intval($_GET['code']) : 200;

// 定义支持的状态码及其描述
$status_codes = [
    200 => ['OK', '请求成功'],
    204 => ['No Content', '无内容'],
    301 => ['Moved Permanently', '永久重定向'],
    302 => ['Found', '临时重定向'],
    400 => ['Bad Request', '错误请求'],
    403 => ['Forbidden', '禁止访问'],
    404 => ['Not Found', '未找到'],
    500 => ['Internal Server Error', '服务器内部错误'],
    502 => ['Bad Gateway', '错误网关'],
    503 => ['Service Unavailable', '服务不可用'],
];

// 验证状态码
if (!isset($status_codes[$code])) {
    $code = 200;
}

list($message, $description) = $status_codes[$code];

// 设置响应状态码
http_response_code($code);
?>
<!DOCTYPE html>
<html>
<head>
    <title>HTTP Status Code: <?php echo $code; ?></title>
</head>
<body>
    <h1>HTTP Status Code Demo</h1>
    
    <div style="font-size: 2em; padding: 20px; border: 2px solid #ccc; margin: 20px 0;">
        <strong><?php echo $code; ?></strong> <?php echo $message; ?>
    </div>
    
    <p><?php echo $description; ?></p>
    
    <h2>测试不同状态码：</h2>
    <ul>
        <?php foreach ($status_codes as $sc => $info): ?>
            <li><a href="statuscode.php?code=<?php echo $sc; ?>">
                <?php echo $sc; ?> - <?php echo $info[0]; ?>
            </a></li>
        <?php endforeach; ?>
    </ul>
    
    <p>使用 F12 → Network 查看响应状态码</p>
</body>
</html>