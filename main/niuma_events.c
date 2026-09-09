#include "niuma_events.h"
#include <string.h>

const nm_event_info_t nm_events[8]={
    {"下班前五分钟","Five minutes to go","领导探头：\n这个今晚能做完吗？","The boss asks:\nCan this be done tonight?",
     {"接下：加班 +16 币","明早做：心情 +8","一起做：关系需 40"},
     {"Overtime: +16 coins","Tomorrow: mood +8","Team up: bond 40"}},
    {"空调温度之争","Thermostat wars","同事觉得冷，\n你却觉得有点热。","One colleague is cold.\nYou feel a little warm.",
     {"商量：沟通 +6","借外套：关系 +8","出去走走：压力 -8"},
     {"Discuss: talk +6","Lend coat: bond +8","Walk: stress -8"}},
    {"电脑正在更新","Updating...","进度条停在 99%。\n它也想休息一下。","The bar stops at 99%.\nEven the PC needs rest.",
     {"喝口水：精力 +6","整理笔记：技能 +6","原地发呆：心情 +6"},
     {"Drink: energy +6","Notes: tech +6","Daydream: mood +6"}},
    {"同事送来点心","A little snack","小羊带了点心，\n给你留了一份。","Your colleague saved\na snack just for you.",
     {"一起吃：饱腹 +12","分享：关系 +10","谢谢：心情 +8"},
     {"Eat: satiety +12","Share: bond +10","Thanks: mood +8"}},
    {"周五团建通知","Friday gathering","下班后还有活动。\n你的时间由你决定。","An after-work gathering.\nYour time, your choice.",
     {"参加：关系 +10","回家：精力 +10","改约午餐：沟通 +6"},
     {"Join: bond +10","Home: energy +10","Lunch instead: talk +6"}},
    {"前辈的建议","A mentor's advice","先做好一件事，\n不用一下变得很厉害。","Start with one thing.\nYou can grow slowly.",
     {"请教：技能 +10","交流：沟通 +10","道谢：关系 +8"},
     {"Learn: tech +10","Discuss: talk +10","Thank: bond +8"}},
    {"我为什么在这里","A bigger question","报表之外，\n我还想做些什么？","Beyond spreadsheets,\nwhat else matters to me?",
     {"散步：压力 -12","找朋友：心情 +12","规划：生活技能 +10"},
     {"Walk: stress -12","Talk: mood +12","Plan: life skill +10"}},
    {"一封感谢邮件","A thank-you note","你昨天帮的那个忙，\n对方一直记得。","Someone remembers\nyour help yesterday.",
     {"收下：心情 +10","回复：关系 +10","记笔记：技能 +6"},
     {"Smile: mood +10","Reply: bond +10","Reflect: tech +6"}}
};
static uint8_t stat_add(unsigned a,int b){int n=(int)a+b;return n<0?0:n>100?100:(uint8_t)n;}
static void skill_add(uint16_t *v,unsigned n){*v=*v>UINT16_MAX-n?UINT16_MAX:(uint16_t)(*v+n);}
nm_error_t nm_event_choose(nm_state_t *s,unsigned choice,nm_effect_t *effect){
    if(effect)memset(effect,0,sizeof(*effect));
    if(choice>=3 || s->event>=8)return NM_BAD_INPUT;
    if(s->event_done)return NM_DAILY_LIMIT;
    nm_state_t next=*s;
    if(s->event==0){
        if(choice==0){nm_error_t e=nm_act(&next,NM_OVERTIME,0,0);if(e!=NM_OK)return e;}
        if(choice==1){next.stat[NM_MOOD]=stat_add(next.stat[NM_MOOD],8);skill_add(&next.skill[1],5);}
        if(choice==2){if(s->relations[0]<40)return NM_LOCKED;next.relations[0]=stat_add(next.relations[0],8);next.stat[NM_STRESS]=stat_add(next.stat[NM_STRESS],-6);}
    }else{
        switch(s->event*3+choice){
        case 3: case 14:skill_add(&next.skill[1],6);break;
        case 4:next.relations[0]=stat_add(next.relations[0],8);break;
        case 5:next.stat[NM_STRESS]=stat_add(next.stat[NM_STRESS],-8);break;
        case 6:next.stat[NM_ENERGY]=stat_add(next.stat[NM_ENERGY],6);break;
        case 7:case 23:skill_add(&next.skill[0],6);break;
        case 8:next.stat[NM_MOOD]=stat_add(next.stat[NM_MOOD],6);break;
        case 9:next.stat[NM_SATIETY]=stat_add(next.stat[NM_SATIETY],12);break;
        case 10:case 12:case 22:next.relations[0]=stat_add(next.relations[0],10);break;
        case 11:next.stat[NM_MOOD]=stat_add(next.stat[NM_MOOD],8);break;
        case 13:next.stat[NM_ENERGY]=stat_add(next.stat[NM_ENERGY],10);break;
        case 15:skill_add(&next.skill[0],10);break;
        case 16:skill_add(&next.skill[1],10);break;
        case 17:next.relations[1]=stat_add(next.relations[1],8);break;
        case 18:next.stat[NM_STRESS]=stat_add(next.stat[NM_STRESS],-12);break;
        case 19:next.stat[NM_MOOD]=stat_add(next.stat[NM_MOOD],12);break;
        case 20:skill_add(&next.skill[2],10);break;
        case 21:next.stat[NM_MOOD]=stat_add(next.stat[NM_MOOD],10);break;
        default:return NM_BAD_INPUT;
        }
    }
    nm_check_friendship(&next);
    next.event_done=1;next.revision++;
    if(effect){
        for(unsigned i=0;i<NM_STAT_COUNT;++i)effect->delta[i]=(int16_t)next.stat[i]-s->stat[i];
        effect->coins=(int32_t)((int64_t)next.coins-s->coins);effect->slots=s->actions-next.actions;
        effect->experience=next.xp-s->xp;
        for(unsigned i=0;i<3;++i){effect->skill[i]=next.skill[i]-s->skill[i];effect->relations[i]=next.relations[i]-s->relations[i];}
    }
    *s=next;return NM_OK;
}
