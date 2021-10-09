#include "mylibpcap.h"

int main()
{
    PCAP_SESSION session("pkts_frag_00000");
    IP_PACKET cur_packet;
    int status = session.get_packet(cur_packet);
    while(status = session.get_packet(cur_packet))
    {
       // cur_packet.show_ip();
        cout<<cur_packet.get_ipstr()<<endl;
        if(session.proc_num()==100)
        {
            break;
        }
    }
    
    return 0;
}