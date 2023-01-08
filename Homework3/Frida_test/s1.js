console.log("Script loaded successfully ");
Java.perform(function x() {
    console.log("Inside java perform function");
    // 1:通过重写stringFromJNI方法来hook
    var myClass = Java.use("com.example.x86demo.MainActivity");
    myClass.stringFromJNI.implementation = function () {
        console.log("Inside stringFromJNI implementation");
        console.log("hook from fqh")
        var retval = "Hello from fqh"
        return retval;
    };
});