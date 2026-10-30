{
  "variables": {
    # Kvaser CANlib SDK folder (see tools/canlib-sdk.js). The SDK is not part
    # of this package; its license does not allow redistributing it.
    "canlib_sdk": "<!(node tools/canlib-sdk.js)"
  },
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
            # canlib32.dll itself is installed by the Kvaser driver package
            "include_dirs": [
              "<(canlib_sdk)/INC"
            ],
            "library_dirs": [
              "<(canlib_sdk)/Lib/x64"
            ],
            "link_settings": {
              "libraries": [
                "-lcanlib32"
              ]
            }
          }
        ]
      ]
    }
  ]
}
