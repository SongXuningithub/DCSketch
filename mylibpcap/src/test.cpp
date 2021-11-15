#include "mylibpcap.h"

int main()
{
    // string dataset = "MAWI";
    string dataset = "KAGGLE";
    string filename = "Dataset-Unicauca";
    PCAP_SESSION session(dataset,filename,CSV_FILE);
    IP_PACKET cur_packet;
    //int status = session.get_packet(cur_packet);
    while(int status = session.get_packet(cur_packet))
    {
       // cur_packet.show_ip();
        //cout<<cur_packet.get_ipstr()<<endl;
        if(session.proc_num() % 100000 == 0)
        {
            cout<<session.proc_num()<<endl;
        }
    }
    
    return 0;
}