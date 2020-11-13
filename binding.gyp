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
                "link_settings": { "libraries": [ "-lcanlib32" ] },
                "conditions": [
                    [ "target_arch=='ia32'", {
                        "library_dirs": [ "ext/canlib-win32-x64/lib/32" ]
                    } ],
                    [ "target_arch=='x64'", {
                        "library_dirs": [ "ext/canlib-win32-x64/lib/64" ]
                    } ]
                ]
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
                    "conditions": [
                        [ "target_arch=='ia32'", {
                            "files": [ "ext/canlib-win32-x64/bin/32/canlib32.dll" ]
                        } ],
                        [ "target_arch=='x64'", {
                            "files": [ "ext/canlib-win32-x64/bin/64/canlib32.dll" ]
                        } ]
                    ]
                } ]
            ],
            "destination": "<(module_path)"
        } ]
    } ]
}

