<?php
$referer = isset($_SERVER['HTTP_REFERER']) ? $_SERVER['HTTP_REFERER'] : '未设置';
?>
<!DOCTYPE html>
<html>
<head>
    <title>Target Page</title>
</head>
<body>
    <h1>你被重定向到了这里！</h1>
    <p>原始请求头中的 Referer 是：<?php echo htmlspecialchars($referer); ?></p>
    
    <?php if ($referer == '未设置'): ?>
        <p style="color: green;">✓ Referrer-Policy: no-referrer 生效，Referer 未被发送</p>
    <?php endif; ?>
</body>
</html>