napi_value canlib_canSetBusParams(napi_env env, napi_callback_info info)
{
    napi_status status = napi_generic_failure;

    // Retrieve callback info and argument list
    size_t argc = CANSETBUSPARAMS_ARGC;
    napi_value argv[CANSETBUSPARAMS_ARGC];

    status = napi_get_cb_info(env, info, &argc, argv, 0, 0);
    assert(status == napi_ok);

    if (argc != CANSETBUSPARAMS_ARGC)
    {
        napi_throw_type_error(env, 0, "Incorrect number of arguments.");
        return 0;
    }

    // Retrieve arguments
    uint32_t canlibHnd;
    status = napi_get_value_uint32(env, argv[0], &canlibHnd);
    assert(status == napi_ok);

    uint32_t canlibFreq;
    status = napi_get_value_uint32(env, argv[1], &canlibFreq);
    assert(status == napi_ok);

    uint32_t canlibTseg1;
    status = napi_get_value_uint32(env, argv[2], &canlibTseg1);
    assert(status == napi_ok);

    uint32_t canlibTseg2;
    status = napi_get_value_uint32(env, argv[3], &canlibTseg2);
    assert(status == napi_ok);

    uint32_t canlibSjw;
    status = napi_get_value_uint32(env, argv[4], &canlibSjw);
    assert(status == napi_ok);

    uint32_t canlibNoSamp;
    status = napi_get_value_uint32(env, argv[5], &canlibNoSamp);
    assert(status == napi_ok);

    uint32_t canlibSyncmode;
    status = napi_get_value_uint32(env, argv[6], &canlibSyncmode);
    assert(status == napi_ok);

    // Set bus parameters
    canStatus canlibStatus;
    canlibStatus = canSetBusParams(canlibHnd,
                                   canlibFreq,
                                   canlibTseg1,
                                   canlibTseg2,
                                   canlibSjw,
                                   canlibNoSamp,
                                   canlibSyncmode);

    // Print results to console
#ifdef CANLIB_DEBUG
    printf("canlib_canSetBusParams:\n"
           "  canlibStatus   = 0x%02X (%s)\n"
           "  canlibFreq     = 0x%02X\n"
           "  canlibTseg1    = 0x%02X\n"
           "  canlibTseg2    = 0x%02X\n"
           "  canlibSjw      = 0x%02X\n"
           "  canlibNoSamp   = 0x%02X\n"
           "  canlibSyncmode = 0x%02X\n",
           canlibStatus, canlibStatusLookup(canlibStatus),
           canlibFreq,
           canlibTseg1,
           canlibTseg2,
           canlibSjw,
           canlibNoSamp,
           canlibSyncmode);
#endif

    // Throw error, if any
    if (canlibStatus < 0)
    {
        napi_throw_error(env, canlibStatusLookup(canlibStatus),
                         "canlib_canSetBusParams: Error at canSetBusParams.");
        return 0;
    }

    // Create an N-API value for the result and return it
    napi_value result;
    status = napi_create_int32(env, canlibStatus, &result);
    assert(status == napi_ok);

    return result;
}
