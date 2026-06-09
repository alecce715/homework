<?php
// 设置响应头，避免重定向时泄露 Referer
header('Referrer-Policy: no-referrer');
// 302 重定向到 target.php
header('Location: target.php', true, 302);
exit;
?>