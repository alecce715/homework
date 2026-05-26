<?php
require 'vendor/autoload.php';
use PHPMailer\PHPMailer\PHPMailer;
use PHPMailer\PHPMailer\Exception;

// 数据库连接
$db = new PDO(
    'mysql:host=localhost;dbname=user_system;charset=utf8mb4',
    'root',
    'your_mysql_password',
    [PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION]
);

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $nickname = trim($_POST['nickname']);
    $password = trim($_POST['password']);
    $email = trim($_POST['email']);

    // 基础校验
    if (empty($nickname) || empty($password) || empty($email)) {
        die('所有字段必填');
    }
    if (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
        die('邮箱格式错误');
    }

    // 检查昵称是否已存在（含未激活的）
    $stmt = $db->prepare("SELECT id FROM users WHERE nickname = ?");
    $stmt->execute([$nickname]);
    if ($stmt->fetch()) {
        die('昵称已被占用');
    }

    // 生成验证码（用户名 + 随机盐值 哈希）
    $salt = bin2hex(random_bytes(16));
    $verifyCode = hash('sha256', $nickname . $salt);
    $expireTime = time() + 3600; // 1小时后过期

    // 插入用户（密码哈希存储）
    $hashedPwd = password_hash($password, PASSWORD_DEFAULT);
    $stmt = $db->prepare(
        "INSERT INTO users (nickname, password, email, verify_code, expire_time, created_at) 
         VALUES (?, ?, ?, ?, ?, ?)"
    );
    $stmt->execute([
        $nickname,
        $hashedPwd,
        $email,
        $verifyCode,
        $expireTime,
        time()
    ]);

    // 发送激活邮件
    sendVerifyEmail($email, $nickname, $verifyCode);
    echo '注册成功！请查收邮件激活账号（1小时内有效）';
}

// 发送邮件函数
function sendVerifyEmail($toEmail, $nickname, $code) {
    $mail = new PHPMailer(true);
    try {
        // SMTP 配置（替换为你的邮箱信息）
        $mail->isSMTP();
        $mail->Host = 'smtp.qq.com';
        $mail->SMTPAuth = true;
        $mail->Username = 'your_qq@qq.com'; // 发件邮箱
        $mail->Password = 'your_smtp_auth_code'; // SMTP 授权码（非邮箱密码）
        $mail->SMTPSecure = PHPMailer::ENCRYPTION_SMTPS;
        $mail->Port = 465;

        // 收件人
        $mail->setFrom('your_qq@qq.com', '系统通知');
        $mail->addAddress($toEmail, $nickname);

        // 邮件内容
        $verifyUrl = "http://127.0.0.1/verify.php?user={$nickname}&code={$code}";
        $mail->isHTML(true);
        $mail->Subject = '账号激活通知';
        $mail->Body = "
            您好 {$nickname}，请点击以下链接激活账号：<br>
            <a href='{$verifyUrl}'>{$verifyUrl}</a><br>
            <strong>链接1小时内有效，超时需重新注册</strong>
        ";

        $mail->send();
    } catch (Exception $e) {
        error_log("邮件发送失败: {$mail->ErrorInfo}");
    }
}
?>
<!-- 注册表单（前端） -->
<form method="POST">
    昵称：<input type="text" name="nickname"><br>
    口令：<input type="password" name="password"><br>
    邮箱：<input type="email" name="email"><br>
    <button type="submit">提交注册</button>
</form>