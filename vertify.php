<?php
$db = new PDO(
    'mysql:host=localhost;dbname=user_system;charset=utf8mb4',
    'root',
    'your_mysql_password',
    [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
);

$user = $_GET['user'] ?? '';
$code = $_GET['code'] ?? '';

if (empty($user) || empty($code)) {
    die('参数错误');
}

// 查询用户
$stmt = $db->prepare(
    "SELECT * FROM users WHERE nickname = ? AND verify_code = ? AND status = 0"
);
$stmt->execute([$user, $code]);
$userInfo = $stmt->fetch(PDO::FETCH_ASSOC);

if (!$userInfo) {
    die('激活链接无效或已使用');
}

// 校验是否过期
if (time() > $userInfo['expire_time']) {
    // 删除过期用户（释放昵称）
    $db->prepare("DELETE FROM users WHERE id = ?")->execute([$userInfo['id']]);
    die('激活链接已过期，请重新注册');
}

// 激活账号
$db->prepare("UPDATE users SET status = 1 WHERE id = ?")->execute([$userInfo['id']]);
echo '账号激活成功！';
?>