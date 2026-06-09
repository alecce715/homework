<?php
// 获取 type 参数，默认为 inline
$type = isset($_GET['type']) ? $_GET['type'] : 'inline';

// 验证参数值
if (!in_array($type, ['inline', 'attachment'])) {
    $type = 'inline';
}

// 测试内容
$content = "这是绝密情报，请在浏览器内直接阅读。\n\n机密级别：最高\n创建时间：" . date('Y-m-d H:i:s') . "\n内容摘要：这是一个测试文件，用于演示 Content-Disposition 头的作用。";

// 根据 type 设置响应头
if ($type == 'attachment') {
    header('Content-Disposition: attachment; filename="confidential.txt"');
} else {
    header('Content-Disposition: inline; filename="report.txt"');
}

// 设置内容类型
header('Content-Type: text/plain; charset=UTF-8');
header('Content-Length: ' . strlen($content));

// 输出内容
echo $content;
?>