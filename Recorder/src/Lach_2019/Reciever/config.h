
#ifndef __CONFIG_H__
#define __CONFIG_H__

const char * source_address = "127.0.0.1";
const char * target_address = "127.0.0.1";
const int port_number = 7891;

//const int n_channels = 8;
//const int samples_per_packet = 16;
//const int timestamp_bytes = 4;
//const int nsamples_bytes = 4;
//const int nchannels_bytes = 1;

const char * files_in [] = {"audio/test1.raw","audio/test2.raw","audio/test3.raw","audio/test4.raw",
                            "audio/test5.raw","audio/test6.raw","audio/test7.raw","audio/test8.raw",
                            "audio/test9.raw","audio/test10.raw","audio/test11.raw","audio/test12.raw",
                            "audio/test13.raw","audio/test14.raw","audio/test15.raw","audio/test16.raw",
                            "audio/test17.raw","audio/test18.raw","audio/test19.raw","audio/test20.raw"};    
const char * files_out [] = {"audio/out_test1.raw","audio/out_test2.raw","audio/out_test3.raw","audio/out_test4.raw",
                             "audio/out_test5.raw","audio/out_test6.raw","audio/out_test7.raw","audio/out_test8.raw",
"audio/out_test9.raw","audio/out_test10.raw","audio/out_test11.raw","audio/out_test12.raw",
"audio/out_test13.raw","audio/out_test14.raw","audio/out_test15.raw","audio/out_test16.raw",
"audio/out_test17.raw","audio/out_test18.raw","audio/out_test19.raw","audio/out_test20.raw",
"audio/out_test21.raw","audio/out_test22.raw","audio/out_test23.raw","audio/out_test24.raw",
"audio/out_test25.raw","audio/out_test26.raw","audio/out_test27.raw","audio/out_test28.raw",
"audio/out_test29.raw","audio/out_test30.raw","audio/out_test31.raw","audio/out_test32.raw"};    

#endif // __CONFIG_H__
