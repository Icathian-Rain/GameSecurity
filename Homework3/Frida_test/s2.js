console.log("Script loaded successfully ");
Java.perform(function x() {
    console.log("Inside java perform function");
    // 2:通过hook setText方法来hook
    // 2.1:获取TextView类
    var tv_class = Java.use("android.widget.TextView");
    // 2.2:重写setText方法
    tv_class.setText.overload("java.lang.CharSequence").implementation = function (x) {
        // 2.3:打印日志
        console.log("Inside setText implementation");
        console.log("hook from fqh!")
        console.log("x:" + x);
        // 判断x是否为"Hello from C++"
        if (x == "Hello from C++")
        // 如果是，就返回"Hello from fqh"
            x = Java.use("java.lang.String").$new("Hello from fqh");
        return this.setText(x);
    };
});