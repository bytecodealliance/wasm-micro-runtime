WASM Graphic Layer (WGL)
=======

The WGL builds the littlevgl v6.0 into the runtime and exports a API layer for WASM appication programming graphic user interfaces. This approach will make the WASM application small footprint. Another option is to build the whole littlevgl library into WASM, which is how the sample littlevgl is implemented. 

# Challenges

When the littlevgl library is compiled into the runtime, all the widget data is actually located in the runtime native space. As the WASM sandbox won't allow the WASM application to directly access the native data, it introduced a few problems for the littlevgl API exporting:

1. Reference to the widget object

   Almost each littlevgl API takes the widget object as the first argument, which leads to either reading or writing the data associated with the object in native space. We have to prevent the WASM app utilize this method to access unauthorized memmory address. 

   The solution is to track the objects created by the WASM App in the native layer. Every access to the object must be validated in advance. To simplify implementing native wrapper function, the wgl_native_func_call() will do the validation for the object presented in the first argument.

   When multiple WASM apps are creating their own UI, the object should also validated for its owner module instance.

2. Access the object properties

   As the data structure of widget objects is no longer visible to the applications, the app has to call APIs to get/set the properties of an object.

3. Pass function arguments in stucture pointers

   We have to do serialization for the structure passing between the WASM and native.

4. Callbacks

# API compatibility with littlevgl
We wish the application continue to use the littlevgl API and keep existing header files inclusion, however it is also a bit challenging since we have to redefine some data types such as lv_obj_t in the APIs exposed to the WASM app. 

'''
typedef void lv_obj_t;
'''



# Prepare the lvgl header files for WASM applicaitons

Run the below script to setup the lvgl header files for the wasm appliation.

```
core/app-framework/wgl/app/prepare_headers.sh
```

The script is also automatically executed after downloading the lvgl repo in the wamr-sdk building process.



# How to extend a little vgl wideget
Currently the wgl has exported the API for a few common used widgets such as button, label etc. 

Refer to the implementation of these widgets for extending more widgets.













































