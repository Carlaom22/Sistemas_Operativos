//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "monitorengine.h"

// #undef DEBUG
// #define DEBUG 1

int wait_time = 30;

void signal_handler_monitor(int signum);

void monitor_engine(){
    signal(SIGUSR1, signal_handler_monitor);

    int current,starting;
    char buffer[MBUF];
    struct timespec timeout;
    backoffice_msg backoffice_msg;
    mobile_msg mobile_msg;
    long next_interval, current_time;
    int alert_level;
    int ret;

    //logs booting
    write_log("[MONITOR ENGINE] BOOTED.",1);

    pthread_mutex_lock(&sync_pointer->mem_mutex);
    while(!end_flag){
        if (DEBUG) write_log("[MONITOR ENGINE] WAITING...",1);
        
        //locks mutex

        //gets time for next interval
        clock_gettime(CLOCK_REALTIME, &timeout);
        current_time = timeout.tv_sec;
        next_interval = (timeout.tv_sec/wait_time + 1) * wait_time;
        timeout.tv_sec = next_interval;

        //logs next interval
        sprintf(buffer,"[MONITOR ENGINE] WAITING FOR SIGNAL, NEXT STATS REPORT IN %ld SECONDS",next_interval - current_time);
        if (DEBUG) write_log(buffer,1);

        ret = pthread_cond_timedwait(&sync_pointer->mem_cond2, &sync_pointer->mem_mutex, &timeout);
        if (DEBUG) write_log("[MONITOR ENGINE] EXITED WAIT...",1);

        if(end_flag == 1){
            pthread_mutex_unlock(&sync_pointer->mem_mutex);
            break;  //exits process loop
        }

        //blocks signals during task
        block_signals();

        //check if time is up
        if(ret == ETIMEDOUT){
            if (1) write_log("[MONITOR ENGINE] SENDING REPORT TO BACKOFFICE...",1);
            //send message to backoffice user
            backoffice_msg.pid = 1;
            backoffice_msg.stats = *info_pointer;

            //prepare stats report
            if (DEBUG){
                printf("\nService\tTotal Data\tAuth Reqs\n");
                printf("VIDEO\t%d\t\t%d\n", backoffice_msg.stats.video_stats.total_data, backoffice_msg.stats.video_stats.auth_requests);
                printf("MUSIC\t%d\t\t%d\n", backoffice_msg.stats.music_stats.total_data, backoffice_msg.stats.music_stats.auth_requests);
                printf("SOCIAL\t%d\t\t%d\n", backoffice_msg.stats.social_stats.total_data, backoffice_msg.stats.social_stats.auth_requests);
            }

                
            if (msgsnd(mqid, &backoffice_msg, sizeof(backoffice_msg) - sizeof(long), IPC_NOWAIT) == -1) {
                if (DEBUG) write_log_error("[MONITOR ENGINE] ERROR AT MSGSND",0,errno);
            }
        }
        else{
            //logs beginning of task
            if (DEBUG) write_log("[MONITOR ENGINE] MONITORING USERS...",1);

            //loop through memory to check if any user reacher limit, locks again when it loops
            for(int i = 0; i < MOBILE_USERS; i++){
                if(mobiles_pointer[i].client_id == -1){
                    continue; //skips to next user
                }
                //printf("Checking user %d with %d/%d \n", mobiles_pointer[i].client_id, mobiles_pointer[i].current_plafond, mobiles_pointer[i].start_plafond);
                current = mobiles_pointer[i].current_plafond;
                starting = mobiles_pointer[i].start_plafond;
                alert_level = (100*(starting - current) / (float) starting);
                if (alert_level >= (mobiles_pointer[i].alert_level+10) &&  alert_level >= 80){
                    mobiles_pointer[i].alert_level = ((alert_level / 10) * 10);
                    sprintf(buffer,"[MONITOR ENGINE] !!! SENDING ALERT FOR USER (ID = %d) !!! ",mobiles_pointer[i].alert_level);
                    write_log(buffer,1);

                    mobile_msg.pid = mobiles_pointer[i].client_id;
                    mobile_msg.alert_val = mobiles_pointer[i].alert_level;

                    if (msgsnd(mqid, &mobile_msg, sizeof(mobile_msg)-sizeof(long), IPC_NOWAIT) == -1) {
                        if (DEBUG) write_log_error("[MONITOR ENGINE] ERROR AT MSGSND",0,errno);
                    }
                    break;  //only alerts for one mobile user
                }
            }
        }

        unblock_signals();
    }
    pthread_mutex_unlock(&sync_pointer->mem_mutex);
    //exiting sequence
    if (DEBUG) write_log("[MONITOR ENGINE] SHUTTING DOWN...",1);
}

void signal_handler_monitor(int signum){
    if(signum == SIGUSR1){
        write_log("[MONITOR ENGINE] RECEIVED SIGUSR1, SHUTTING DOWN...",1);
        end_flag = 1;
        pthread_cond_signal(&sync_pointer->mem_cond2); //sending cond signal to stop the timed wait immediately, if it was there (otherwise it does nothing)
    }
}