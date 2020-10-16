{
    "targets": [ {
        "target_name": "<(module_name)",
        "sources": [ "src/cs_canlib.c",
                     "src/cs_canlib_event.c",
                     "src/cs_canlib_helper.c",
                     "src/napi_helper.c",
                     "src/common_helper.c" ],
        "conditions": [
            [ "OS=='win'", {
                "sources": [ ],
                "include_dirs": [ "ext/canlib-win32-x64/include" ],
                "library_dirs": [ "ext/canlib-win32-x64/lib" ],
                "link_settings": { "libraries": [ "-lcanlib32" ] }
            } ]
        ]
    },
    {
        "target_name": "action_after_build",
        "type": "none",
        "dependencies": [ "<(module_name)" ],
        "copies": [ {
            "files": [ "<(PRODUCT_DIR)/<(module_name).node" ],
            "conditions": [
                [ "OS=='win'", {
                    "files": [
                        "ext/canlib-win32-x64/bin/canlib32.dll",
                        "ext/canlib-win32-x64/lib/canlib32.lib"
                    ]
                } ]
            ],
            "destination": "<(module_path)"
        } ]
    } ]
}

