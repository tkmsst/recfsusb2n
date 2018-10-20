// Channel conversion table
// For BS/CS setting, refer to
// http://www5e.biglobe.ne.jp/~kazu_f/digital-sat/index.html
//

#define MAX_CH 256

typedef struct _Channel_conv_table {
    char channel[16];
    int freq;		// frequency no.
    int sid;		// Service ID
    int tsid;		// TS ID
} Channel_conv_table;

Channel_conv_table channel_table[MAX_CH];

int channel_conv(char* channel);
