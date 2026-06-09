<?php
$method = $_SERVER['REQUEST_METHOD'];
?>
<!DOCTYPE html>
<html>
<head>
    <title>Method Test</title>
</head>
<body>
    <h1>HTTP 请求方法测试</h1>
    <p>当前请求方法：<?php echo $method; ?></p>
    
    <h2>GET 参数：</h2>
    <pre><?php print_r($_GET); ?></pre>
    
    <h2>POST 参数：</h2>
    <pre><?php print_r($_POST); ?></pre>
    
    <h2>PUT 数据：</h2>
    <pre><?php 
        if ($method == 'PUT') {
            $put_data = file_get_contents('php://input');
            echo $put_data;
        } else {
            echo "当前请求不是 PUT 方法";
        }
    ?></pre>
    
    <form method="GET" action="method_test.php">
        <input type="text" name="get_param" placeholder="GET 参数">
        <button type="submit">发送 GET 请求</button>
    </form>
    
    <form method="POST" action="method_test.php">
        <input type="text" name="post_param" placeholder="POST 参数">
        <button type="submit">发送 POST 请求</button>
    </form>
    
    <p>使用 curl 测试 PUT：</p>
    <code>curl -X PUT -d "put_data=test" http://localhost/method_test.php</code>
</body>
</html>