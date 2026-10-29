{
  "targets": [
    {
      "target_name": "cs_canlib",
      "sources": [
        "src/cs_canlib.c",
        "src/cs_canlib_event.c",
        "src/cs_canlib_helper.c",
        "src/napi_helper.c",
        "src/common_helper.c"
      ],
      "conditions": [
        [
          "OS=='win'",
          {
            # Kvaser CANlib SDK (64-bit)
            "include_dirs": [
              "ext/canlib-win32-x64/include"
            ],
            "library_dirs": [
              "ext/canlib-win32-x64/lib/64"
            ],
            "link_settings": {
              "libraries": [
                "-lcanlib32"
              ]
            },
            # Put canlib32.dll next to the built .node file. Node loads addons
            # with LOAD_WITH_ALTERED_SEARCH_PATH, so Windows finds the DLL in
            # the addon's own folder first. (tools/copy-dll.js does the same
            # for prebuilds/win32-x64/.)
            "copies": [
              {
                "destination": "<(PRODUCT_DIR)",
                "files": [
                  "ext/canlib-win32-x64/bin/64/canlib32.dll"
                ]
              }
            ]
          }
        ]
      ]
    }
  ]
}
