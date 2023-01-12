using UnityEngine;
using System.Collections;
using System.Reflection;
using System;

namespace InjectDll
{
    public static class OperatePrivate
    {
    public static T GetPrivateField<T>(this object instace, string fieldName)
        {
            BindingFlags flag = BindingFlags.Instance | BindingFlags.NonPublic;
            Type type = instace.GetType();
            FieldInfo field = type.GetField(fieldName, flag);
            return (T)field.GetValue(instace);
        }
    //获得私有属性的值
    public static T GetPrivateProperty<T>(this object obj, string propertyName)
        {
            PropertyInfo property = obj.GetType().GetProperty(propertyName, BindingFlags.NonPublic | BindingFlags.Instance);
            return (T)property.GetValue(obj, null);
        }
    //设置私有成员的值
    public static void SetPrivateField(this object obj, string fieldName, object value)
        {
            FieldInfo field = obj.GetType().GetField(fieldName, BindingFlags.NonPublic | BindingFlags.Instance);
            field.SetValue(obj, value);
        }
    //设置私有属性的值
    public static void SetPrivateProperty(this object obj, string propertyName, object value)
        {
            PropertyInfo property = obj.GetType().GetProperty(propertyName, BindingFlags.NonPublic | BindingFlags.Instance);
            property.SetValue(obj, value, null);
        }
    //调用私有方法
    public static object InvokePrivateMethod(this object obj, string methodName, object[] parameters)
        {
            MethodInfo method = obj.GetType().GetMethod(methodName, BindingFlags.
            NonPublic | BindingFlags.Instance);
            return method.Invoke(obj, parameters);
        }
    }
    public class Cheat : UnityEngine.MonoBehaviour
    {
        private void OnGUI()
        {
            UnityEngine.GUI.Label(new Rect(0, 0, 100, 100), "Hack!");
        }
        public void FixedUpdate()
        {
            if (UnityEngine.Input.GetKeyDown(KeyCode.F1))
            {
                //分数 + 1000
                var bs = UnityEngine.GameObject.FindWithTag("Player").GetComponent<BirdScripts>();
                if (bs != null)
                {
                    bs.score = bs.score + 1000;
                }
            }
            if (UnityEngine.Input.GetKeyDown(KeyCode.F2))
            {
                // 无敌
                var Player = UnityEngine.GameObject.FindWithTag("Player");
                var bs = Player.GetComponent<BirdScripts>();
                Player.GetComponent<Collider2D>().isTrigger = true;
            }
            if (UnityEngine.Input.GetKeyDown(KeyCode.F3))
            {
                //取消无敌
                var Player = UnityEngine.GameObject.FindWithTag("Player");
                var bs = Player.GetComponent<BirdScripts>();
                Player.GetComponent<Collider2D>().isTrigger = false;
            }
            if (UnityEngine.Input.GetKeyDown(KeyCode.F4))
            {
                var Pipes = UnityEngine.GameObject.FindGameObjectsWithTag("Pipe");
                // 输出管道数量
                UnityEngine.Debug.Log(Pipes.Length);
                foreach (var pipe in Pipes)
                {
                    // 删除所有管道，可行
                    GameObject.Destroy(pipe);
                    // 将管道的碰撞体设置为触发器，可行
                    //try
                    //{
                    //    pipe.GetComponent<Collider2D>().isTrigger = true;
                    //}
                    //catch (Exception e)
                    //{
                    //    UnityEngine.Debug.Log(e);
                    //}
                    //将管道的2D碰撞模型设置为不启用,可行
                    //try
                    //{
                    //    pipe.GetComponent<Collider2D>().enabled = false;
                    //}
                    //catch (Exception e)
                    //{
                    //    UnityEngine.Debug.Log(e);
                    //}
                }
            }
        }
    }
}

