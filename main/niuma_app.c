#include "niuma_app.h"
#include "niuma_view.h"
#include "niuma_events.h"
#include "niuma_sound.h"
#include "bsp_display.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <stdio.h>
#include <string.h>

typedef enum { P_BOOT,P_CREATE,P_NAME,P_HELP,P_HOME,P_MORE,P_WORK,P_FOOD,P_REST,
    P_STUDY,P_CARE,P_SOCIAL,P_FRIEND,P_ACTION,P_RESULT,P_EVENT,P_GAMES,P_GAME,
    P_STATS,P_CAREER,P_STYLE,P_REVIEW,P_RESIGN,P_COMPANY,P_SHOP,P_CATEGORY,P_ITEM,
    P_BAG,P_ALBUM,P_MEMORY,P_SETTINGS,P_SOUND,P_BRIGHT,P_IDLE,P_LANGUAGE,P_SAVE,
    P_RESET,P_PAY,P_GAME_PAUSE,P_BADGE,P_ROUTES,P_ROUTE,P_SAVE_STATUS,P_GAME_GUIDE,P_START_ERROR,P_DEVICE,P_EVENT_RESULT,P_BURNOUT,P_LEAVING,P_NOTEBOOK,P_MUSIC,P_MUSIC_RESULT,P_POOR,P_EXHAUSTED,P_MORNING } page_t;
typedef struct {bsp_btn_t key;bsp_btn_ev_t event;} key_t;
static nm_state_t state;
static nm_game_t game;
static nm_view_t view;
static nm_effect_t last_effect;
static nm_receipt_t receipt;
static page_t page=P_BOOT,stack[16];
static unsigned depth,selection,friend_id,category,item,food,achievement,frame,style_choice;
static nm_route_t route_choice;
static nm_game_kind_t game_choice;
static const char *game_zh[]={"键盘冲刺","会议回应","通勤抢座","茶水摸鱼","接住便当","下班冲刺"};
static const char *game_en[]={"Typing","Meeting","Commute","Tea break","Bento catch","Going home"};
static const char *meeting_zh[]={"同事刚完成了一个难题。","有人在说明新的操作步骤。","任务要求说得有些模糊。","大家已经确认了分工。","汇报中出现了重要日期。","两项任务的期限冲突了。"};
static const char *meeting_en[]={"A colleague solved a hard task.","New steps are being explained.","The task is not clearly defined.","The team agrees on who does what.","An important date is announced.","Two deadlines clash."};
static const char *response_zh[]={"点头","记笔记","提问"};
static const char *response_en[]={"Nod","Take notes","Ask"};
static const char *route_zh[]={"普通职场","技术专家","团队管理","自由职业","经营小店","舒适生活"};
static const char *route_en[]={"Office life","Technical expert","Team manager","Freelancer","Small shop","Comfortable life"};
static const char *company_zh[]={"稳稳公司","项目团队","弹性工作室","休整中"};
static const char *company_en[]={"Stable Co.","Project team","Flex studio","Between jobs"};
static const char *name_zh[]={"小牛","阿马","小麦","小豆"};
static const char *name_en[]={"Niu","Ma","Mai","Dou"};
static const char *memory_zh[]={"第一份工资","准点下班十次","第一次涨薪","主动离职","生活专家","画饼券","新的好朋友","第一次做饭"};
static const char *memory_en[]={"First pay","Ten on-time days","First raise","Resignation","Life expert","Empty promise","New friend","First cooking"};
static nm_action_t pending_action;
static nm_scene_t result_scene;
static char result_title[80],result_body[512];
static bool loaded,started,sleeping,adjusting,game_paid,game_press_pending;
static bool save_request_failed;
static uint8_t adjustment_original;
static uint32_t last_tick,last_input,last_draw;
static QueueHandle_t keys;
static const char *tr(const char *zh,const char *en){return state.english?en:zh;}
static void copy(char *out,size_t size,const char *s){snprintf(out,size,"%s",s);}
static void option(const char *zh,const char *en){if(view.count<12)copy(view.options[view.count++],100,tr(zh,en));}
static void title(const char *zh,const char *en){copy(view.title,sizeof(view.title),tr(zh,en));}
static void save(void){
    if(!loaded)return; /* No default-character autosave before explicit creation. */
    nm_state_t snapshot=state;
    /* Inactivity saves must not commit an unconfirmed preview. */
    if(adjusting && page==P_BRIGHT)snapshot.brightness=adjustment_original;
    if(adjusting && page==P_SOUND)snapshot.volume=adjustment_original;
    save_request_failed=!nm_storage_request(&snapshot);
}
static void cancel_adjustment(void){
    if(adjusting && page==P_BRIGHT){state.brightness=adjustment_original;bsp_display_backlight(state.brightness);}
    if(adjusting && page==P_SOUND)state.volume=adjustment_original;
    adjusting=false;
}
static void go(page_t next){cancel_adjustment();if(depth<16)stack[depth++]=page;page=next;selection=0;if(next==P_STYLE)style_choice=nm_style(&state);if(next==P_BRIGHT){adjustment_original=state.brightness;adjusting=true;}}
static void home(void){cancel_adjustment();game_press_pending=false;page=loaded?P_HOME:P_BOOT;selection=0;depth=0;}
static void back(void){cancel_adjustment();page=depth?stack[--depth]:loaded?P_HOME:P_BOOT;selection=0;}
static void result(const char *zh,const char *en,const char *body,nm_scene_t scene){
    copy(result_title,sizeof(result_title),tr(zh,en));copy(result_body,sizeof(result_body),body);result_scene=scene;go(P_RESULT);
}
static void error(nm_error_t e){
    static const char *zh[]={"完成","行动已用完，结算后休息吧。","零钱不足，可选免费基础餐或休养。","今天已经用过了。","先休息一下，再来工作。","尚未满足条件。","无法执行该操作。"};
    static const char *en[]={"Done","No actions. Finish the day to rest.","Not enough coins. Basic meals and recovery are free.","Already used today.","Rest before working.","Requirements not met.","Invalid action."};
    nm_audio_play(NM_SFX_ERROR);result("暂时不能执行","Not available",tr(zh[e],en[e]),NM_SCENE_STAND);
    if(e==NM_NO_MONEY)page=P_POOR;
    else if(e==NM_NO_ACTIONS)page=P_EXHAUSTED;
}
static void append_growth(char *text,size_t capacity,nm_action_t a,const nm_effect_t *effect){
    size_t n=strlen(text);
    if((a>=NM_STUDY_TECH && a<=NM_STUDY_LIFE) || effect->skill[0] || effect->skill[1] || effect->skill[2])snprintf(text+n,capacity-n,tr("\n专业 +%u 沟通 +%u 生活 +%u","\nTech +%u Talk +%u Life +%u"),effect->skill[0],effect->skill[1],effect->skill[2]);
    else if(a==NM_CHAT || a==NM_GIFT || effect->relations[0] || effect->relations[1] || effect->relations[2])snprintf(text+n,capacity-n,tr("\n小羊 +%u 前辈 +%u 小鹿 +%u","\nYang +%u Mentor +%u Lu +%u"),effect->relations[0],effect->relations[1],effect->relations[2]);
}
static const char *review_hint(void){
    static char text[160];
    if(state.company==NM_UNEMPLOYED)return tr("先加入一家公司再谈薪。","Join an employer to negotiate.");
    if(state.salary>=100)return tr("日薪已到 100，不再重复谈薪。","Base pay is capped at 100.");
    unsigned days=nm_review_wait_days(&state);
    if(days){snprintf(text,sizeof(text),tr("距离下次谈薪还有 %u 天。","Next review in %u days."),days);return text;}
    return tr("可谈薪，准备沟通 24、经验 120。","Ready. Aim for Talk 24, XP 120.");
}
static void action_result(nm_action_t a){
    char text[320];
    snprintf(text,sizeof(text),tr("饱腹 %+d 精力 %+d\n心情 %+d 压力 %+d\n健康 %+d 金币 %+ld\n经验 +%u 行动 -%u","Food %+d Energy %+d\nMood %+d Stress %+d\nHealth %+d Coins %+ld\nXP +%u Actions -%u"),
        last_effect.delta[0],last_effect.delta[1],last_effect.delta[2],last_effect.delta[3],last_effect.delta[4],(long)last_effect.coins,last_effect.experience,last_effect.slots);
    append_growth(text,sizeof(text),a,&last_effect);
    nm_scene_t scene=nm_art_action_scene(a);
    result(nm_actions[a].zh,nm_actions[a].en,text,scene);
    nm_audio_play(a>=NM_BASIC_MEAL?NM_SFX_EAT:a==NM_WORK?NM_SFX_WORK:NM_SFX_REST);save();
}
static void prepare_action(nm_action_t a){pending_action=a;go(P_ACTION);if(a==NM_OVERTIME)selection=1;}
static const char *action_requirement(nm_action_t a,nm_error_t err){
    if(err==NM_OK)return tr("确认后才生效。","Confirm to apply.");
    if(err==NM_NO_ACTIONS)return tr("行动用完，结束今天后睡觉。","No actions. Finish today and sleep.");
    if(err==NM_NO_MONEY)return tr("金币不足，可选免费基础餐。","Need coins. Basic meal is free.");
    if(err==NM_DAILY_LIMIT)return tr("这项行动每天只能一次。","This action is once per day.");
    if(err==NM_TOO_TIRED)return tr("请先休养，恢复精力和健康。","Recover energy and health first.");
    if(a==NM_COOK)return tr("需要生活技能达到 24。","Requires Life skill 24.");
    if(state.evening)return tr("已经下班，明天再工作。","Work is closed. Try tomorrow.");
    if(nm_weekend(&state))return tr("周末不工作，好好休息。","No work on weekends. Rest well.");
    if(a==NM_OVERTIME && (state.route==NM_ROUTE_FREELANCE || state.route==NM_ROUTE_SHOP))return tr("当前职业不提供加班。","No overtime in this direction.");
    return tr("先入职或选择自由职业、小店。","Join a company or choose self-employment.");
}
static nm_action_t game_action(nm_game_kind_t kind){
    return kind==NM_GAME_TYPING?NM_WORK:kind==NM_GAME_TEA?NM_SLACK:kind==NM_GAME_BENTO?NM_BASIC_MEAL:kind==NM_GAME_MEETING?NM_STUDY_TALK:NM_EXERCISE;
}
static void prepare_game(nm_game_kind_t kind){game_choice=kind;go(P_GAME_GUIDE);}
static void start_game(nm_game_kind_t kind){
    nm_action_t action=game_action(kind);
    nm_error_t err=nm_act(&state,action,friend_id,&last_effect);if(err!=NM_OK){error(err);return;}
    nm_game_start(&game,kind,nm_random(&state));game_paid=true;game_press_pending=false;save();go(P_GAME);
}
static const char *game_cue(void){
    if(game.kind==NM_GAME_TEA){
        if(nm_game_danger(&game))return game.busy?tr("保持忙碌，别喝茶！","Stay busy. Do not sip!"):tr("被发现了，下轮再试。","Caught. Try next round.");
        if(nm_game_warning(&game))return game.busy?tr("脚步接近，保持忙碌","Footsteps near. Stay busy."):tr("脚步接近！确定装忙","Warning! OK: act busy");
        return game.busy?tr("安全，确定开始喝茶","Safe. OK: start sipping"):tr("正在喝茶，留意脚步","Sipping. Watch for warning");
    }
    if(game.used)return game.feedback==1?tr("成功！等待下一轮","Success! Wait for next round"):tr("错过了，下轮再试","Missed. Try next round");
    return nm_game_window(&game)?tr("现在按确定！","Press OK now!"):tr("等待时机…","Wait for the cue...");
}
static void describe(void){
    memset(&view,0,sizeof(view));view.kind=NM_VIEW_MENU;view.scene=NM_SCENE_STAND;
    switch(page){
    case P_BOOT:title("牛马歌子","NiuMa");view.kind=NM_VIEW_SCENE;
        if(!started){copy(view.body,sizeof(view.body),tr("正在读取存档，请稍候。","Loading your save. Please wait."));break;}
        copy(view.body,sizeof(view.body),tr("养一位属于你的职场伙伴。","Raise your own office buddy."));option("继续游戏","Continue");option("新员工入职","New employee");break;
    case P_START_ERROR:title("存档未能读取","Cannot load save");view.kind=NM_VIEW_TEXT;
        copy(view.body,sizeof(view.body),tr("旧进度尚未恢复。\n不会自动覆盖旧存档。\n请先尝试重新读取。\n重新开始需要再次确认。","Your old save was not loaded.\nIt will not be overwritten\nautomatically. Retry first.\nStarting over needs confirmation."));
        option("重新读取，不写入","Retry read only");option("确认重新开始","Start over...");break;
    case P_CREATE:title("选择角色","Choose character");view.kind=NM_VIEW_SCENE;copy(view.body,sizeof(view.body),state.gender?tr("女性：高马尾、短鹅蛋脸","Female: ponytail, rounded face"):tr("男性：偏分短发、宽方脸","Male: side part, square face"));option("切换男女外观","Switch appearance");option("下一步","Next");break;
    case P_NAME:title("员工姓名","Employee name");view.kind=NM_VIEW_SCENE;copy(view.body,sizeof(view.body),state.name==0?tr("小牛","Niu"):state.name==1?tr("阿马","Ma"):state.name==2?tr("小麦","Mai"):tr("小豆","Dou"));option("换个名字","Next name");option("领取工牌","Get badge");break;
    case P_HELP:title("入职指南","Employee guide");view.kind=NM_VIEW_TEXT;copy(view.body,sizeof(view.body),tr("上下键选择，确定键进入。\n长按确定返回。\n每天六格行动，关机不掉状态。\n记得吃饭，也记得休息！","UP/DOWN: select. OK: enter.\nHold OK: back.\nSix actions each day.\nNo offline penalties.\nEat well and remember to rest."));option("开始今天","Start the day");break;
    case P_MORNING:{
        static const char *zh[]={"星期一","星期二","星期三","星期四","星期五","星期六","星期日"};
        static const char *en[]={"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
        title("新的一天","A new day");view.kind=NM_VIEW_SCENE;view.scene=nm_weekend(&state)?NM_SCENE_PARK:NM_SCENE_HOME;
        snprintf(view.body,sizeof(view.body),tr("第 %lu 周 %s\n精力 %u 饱腹 %u\n今天有六格行动。\n%s","Week %lu | %s\nEnergy %u Food %u\nSix actions for today.\n%s"),(unsigned long)((state.day-1)/7+1),tr(zh[(state.day-1)%7],en[(state.day-1)%7]),state.stat[NM_ENERGY],state.stat[NM_SATIETY],nm_weekend(&state)?tr("今天不工作，好好生活。","No work today. Enjoy life."):tr("慢慢来，也记得照顾自己。","Take your time. Care for yourself."));
        option("开始今天","Start today");option("查看员工资料","View employee stats");break;}
    case P_HOME:title(state.evening?"下班后的家":"牛马歌子",state.evening?"Home sweet home":"NiuMa");view.kind=NM_VIEW_HOME;view.scene=state.evening||state.company==NM_UNEMPLOYED?NM_SCENE_HOME:nm_weekend(&state)?NM_SCENE_PARK:NM_SCENE_OFFICE;
        snprintf(view.body,sizeof(view.body),tr("饱腹 %u 精力 %u\n心情 %u 压力 %u 健康 %u\n行动 %u/6  金币 %lu","Food %u Energy %u\nMood %u Stress %u HP %u\nACT %u/6  Coins %lu"),state.stat[0],state.stat[1],state.stat[2],state.stat[3],state.stat[4],state.actions,(unsigned long)state.coins);
        if(state.evening)option("睡觉","Sleep");
        else if(nm_weekend(&state)){title("周末生活日","Weekend");option("散步","Walk");}
        else if(state.route==NM_ROUTE_FREELANCE)option("接单","Work");
        else if(state.route==NM_ROUTE_SHOP)option("开店","Shop");
        else if(state.company==NM_UNEMPLOYED)option("去向","Paths");
        else option("工作","Work");
        option("吃饭","Food");option("休息","Rest");option("社交","Chat");option("更多","More");break;
    case P_MORE:title("更多行动","More");option("员工资料","Employee stats");option("学习成长","Study");option("运动整理治疗","Personal care");option("今日事件","Today's event");option("小游戏","Minigames");option("商店与背包","Shop & inventory");option("纪念册","Memories");option("系统设置","Settings");option("结束今天","Finish today");break;
    case P_WORK:title("工作","Work");option("处理任务：绩效 +10","Work: performance +10");option("键盘小游戏","Typing game");option("参加会议","Meeting game");option("自愿加班：压力 +20","Overtime: stress +20");option("下班结算","Finish day");break;
    case P_FOOD:title("食堂","Canteen");for(unsigned i=NM_WATER;i<NM_ACTION_COUNT;++i)option(nm_actions[i].zh,nm_actions[i].en);option("查看食品背包","Food inventory");break;
    case P_REST:title("休息","Rest");option("闭眼小憩","Take a nap");option("茶水间摸鱼","Tea break");option("摸鱼小游戏","Tea-break game");option("回家休息","Recovery at home");option("结束今天","Finish day");option("听一会儿音乐","Listen to music");break;
    case P_STUDY:title("学习","Study");for(unsigned i=NM_STUDY_TECH;i<=NM_STUDY_LIFE;++i)option(nm_actions[i].zh,nm_actions[i].en);option("学习笔记","Learning notebook");break;
    case P_NOTEBOOK:title("学习笔记","Learning notebook");view.kind=NM_VIEW_TEXT;
        snprintf(view.body,sizeof(view.body),tr("专业 %u\n沟通 %u\n生活 %u\n职业经验 %u\n做饭需要生活技能 24。\n技能不会因关机而减少。","Tech %u\nTalk %u\nLife %u\nCareer XP %u\nCooking needs Life 24.\nSkills stay while powered off."),state.skill[0],state.skill[1],state.skill[2],state.xp);
        option("继续学习","Back to study");option("查看职业成长","Career growth");break;
    case P_CARE:title("照顾自己","Personal care");option("出去散步","Exercise");option("清理桌面","Tidy desk");option("预约治疗：20 币","Treatment: 20 coins");option("免费休养","Free recovery");option("带薪思考","Quiet moment");break;
    case P_SOCIAL:title("同事好友","Friends");option("隔壁小羊","Yang next door");option("前辈阿马","Mentor Ma");option("饭搭子小鹿","Lunch buddy Lu");break;
    case P_FRIEND:title("朋友互动","Friend");view.kind=NM_VIEW_SCENE;view.scene=NM_SCENE_FRIEND;snprintf(view.body,sizeof(view.body),tr("关系 %u/100\n一起聊聊今天吧。","Bond %u/100\nLet's talk about today."),state.relations[friend_id]);option("聊聊今天","Chat");option("送点心：8 币","Gift: 8 coins");break;
    case P_ACTION:case P_MUSIC:{const nm_action_info_t *a=&nm_actions[pending_action];title(a->zh,a->en);view.kind=NM_VIEW_TEXT;
        nm_state_t preview=state;nm_effect_t effect;nm_error_t err=nm_act(&preview,pending_action,friend_id,&effect);
        if(err==NM_OK)snprintf(view.body,sizeof(view.body),tr("价格 %u 币 行动 %u 格\n饱腹 %+d 精力 %+d\n心情 %+d 压力 %+d\n健康 %+d 经验 +%u\n金币净变化 %+ld\n确认后才生效。","Cost %u coins, %u action\nFood %+d Energy %+d\nMood %+d Stress %+d\nHealth %+d XP +%u\nNet coins %+ld\nConfirm to apply."),a->cost,a->slots,effect.delta[0],effect.delta[1],effect.delta[2],effect.delta[3],effect.delta[4],effect.experience,(long)effect.coins);
        else snprintf(view.body,sizeof(view.body),tr("价格 %u 币 行动 %u 格\n%s","Cost %u coins, %u action\n%s"),a->cost,a->slots,action_requirement(pending_action,err));
        if(err==NM_OK)append_growth(view.body,sizeof(view.body),pending_action,&effect);
        if(page==P_MUSIC){title("听一会儿音乐","Listen to music");size_t n=strlen(view.body);snprintf(view.body+n,sizeof(view.body)-n,"\n%s",tr("原创音乐，退出即停止试听。","Original tune; leave to stop."));}
        option("确认执行","Confirm");option("取消","Cancel");if(pending_action>=NM_BENTO&&pending_action<=NM_MILK_TEA)option("只买入背包","Buy for later");break;}
    case P_RESULT:case P_EVENT_RESULT:case P_MUSIC_RESULT:title(result_title,result_title);view.kind=page==P_EVENT_RESULT?NM_VIEW_TEXT:NM_VIEW_SCENE;view.scene=result_scene;copy(view.body,sizeof(view.body),result_body);
        if(page==P_MUSIC_RESULT){title("听歌休息中","Music break");size_t n=strlen(view.body);snprintf(view.body+n,sizeof(view.body)-n,"\n%s",state.volume?nm_audio_ready()?tr("原创旋律，静静听一会儿。","An original tune. Take a breath."):tr("音频不可用，仍可安静休息。","No audio; rest still helps."):tr("当前静音，休息收益不变。","Muted; rest benefits stay."));}
        option("继续","Continue");break;
    case P_EVENT:{const nm_event_info_t *e=&nm_events[state.event];title(e->zh,e->en);view.kind=NM_VIEW_TEXT;
        if(state.event_done){copy(view.body,sizeof(view.body),tr("今天的事件已经处理。\n明天再来看看吧。","Today's event is complete.\nCome back tomorrow."));option("返回","Back");}
        else{copy(view.body,sizeof(view.body),tr(e->body_zh,e->body_en));for(unsigned i=0;i<3;++i)option(e->choice_zh[i],e->choice_en[i]);}break;}
    case P_GAMES:title("小游戏","Minigames");option("键盘冲刺：工作一格","Typing: work action");option("会议回应：学习一格","Meeting: study action");option("通勤抢座：活动一格","Commute: activity");option("茶水摸鱼：休息一格","Tea: break action");option("接住便当：吃饭一格","Bento: meal action");option("下班冲刺：活动一格","Escape: activity");break;
    case P_GAME_GUIDE:{
        static const char *zh[]={"上下选择目标行。\n游标进入中间区时按确定。","每题八秒，阅读议题。\n上下选点头、笔记或提问。\n合适回应有成长收益。","上下选择亮起的空座。\n空座出现时按确定坐下。","开始时自动喝茶。\n看到预警就按确定装忙。\n下一轮确定切回喝茶。","上下移动托盘接便当。\n便当落到底部时按确定。","下三层，每层两币。\n电梯开门下两层，楼梯一层。\n两轮内出门再奖两币。"};
        static const char *en[]={"UP/DN: select target row.\nOK when cursor is centered.","Read each topic: 8 seconds.\nUP/DN: nod, note or ask.\nGood responses help you grow.","UP/DN: choose lit seat.\nOK while the seat is free.","Start sipping automatically.\nWarning: OK to act busy.\nNext round: OK to sip again.","UP/DN: move the tray.\nOK as the bento reaches it.","3 floors: 2 coins per floor.\nOpen lift: down 2; stairs: 1.\nExit in 2 rounds: +2 coins."};
        title(game_zh[game_choice],game_en[game_choice]);view.kind=NM_VIEW_TEXT;
        const nm_action_info_t *a=&nm_actions[game_action(game_choice)];
        snprintf(view.body,sizeof(view.body),tr("%s\n基础行动：%s\n消耗 %u 格，%u 币\n最多 8 轮，额外最多 8 币。\n长按暂停，可跳过。","%s\nBase action: %s\nCost: %u action, %u coins\nUp to 8 rounds / 8 extra coins.\nHold to pause / skip."),tr(zh[game_choice],en[game_choice]),tr(a->zh,a->en),a->slots,a->cost);
        option("开始小游戏","Play minigame");option("只做基础行动","Base action only");option("返回，不消耗行动","Back without spending");break;}
    case P_BURNOUT:
        title(state.burnout?"需要缓一缓":"慢慢恢复了",state.burnout?"Time to slow down":"Feeling better");view.kind=NM_VIEW_TEXT;
        if(state.burnout){
            copy(view.body,sizeof(view.body),tr("最近几天有点累。\n这不是你的错。\n先暂停加班，照顾自己。\n连续两天睡前压力不高于 35，\n就能走出倦怠。","The past few days were tiring.\nThis is not your fault.\nPause overtime and take care.\nEnd two days at stress 35\nor below to recover."));
            option("安排免费休养","Plan free recovery");option("找朋友聊聊","Talk with a friend");option("先看看今天","See today's plans");
        }else{copy(view.body,sizeof(view.body),tr("休养开始有了效果。\n已经走出倦怠。\n不用急着把进度追回来。\n今天也按自己的节奏生活。","Taking care helped.\nBurnout has lifted.\nNo need to rush to catch up.\nKeep your own pace today."));option("慢慢开始新一天","Start the day gently");}break;
    case P_GAME:{static const char *zh[]={"键盘冲刺","会议回应","通勤抢座","茶水摸鱼","接住便当","下班冲刺"};static const char *en[]={"Typing","Meeting","Commute","Tea break","Bento catch","Going home"};title(zh[game.kind],en[game.kind]);view.kind=NM_VIEW_GAME;
        if(game.kind==NM_GAME_MEETING){
            view.kind=NM_VIEW_MEETING;
            const char *cue=game.used?(game.feedback==1?tr("回应合适！","Good response!"):tr("下次试试：","Next time: ")):tr("请按议题选择回应。","Choose a fitting response.");
            snprintf(view.body,sizeof(view.body),tr("议题 %u/8  剩余 %lu 秒\n%s\n%s%s","Topic %u/8  %lu seconds\n%s\n%s%s"),game.round+1,(unsigned long)((8000-game.round_ms+999)/1000),tr(meeting_zh[game.topic],meeting_en[game.topic]),cue,game.used&&game.feedback==2?tr(response_zh[game.target],response_en[game.target]):"");
            option("点头：关系 +2","Nod: bond +2");option("记笔记：经验 +3","Notes: XP +3");option("提问：沟通 +2 精力 -1","Ask: talk +2 energy -1");
            copy(view.hint,sizeof(view.hint),tr("合适回应才生效 长按暂停","Fitting answers earn rewards"));break;
        }
        if(game.kind==NM_GAME_ESCAPE){
            view.kind=NM_VIEW_ESCAPE;char lift[120];
            if(game.used)copy(lift,sizeof(lift),game.feedback==1?tr("已下楼，下轮继续。","Descended. Next round soon."):tr("没有上车，下轮再试。","Missed the lift. Try next round."));
            else if(game.lift_meeting)copy(lift,sizeof(lift),tr("电梯口临时会议！\n走楼梯可以绕开。","Meeting by the lift!\nTake the stairs to bypass it."));
            else if(nm_game_lift_ready(&game))copy(lift,sizeof(lift),tr("电梯门开着，现在确认！","Lift open. Confirm now!"));
            else if(game.round_ms<game.lift_arrival_ms)snprintf(lift,sizeof(lift),tr("电梯还有 %lu 秒到达。\n等开门再确认。","Lift arrives in %lu seconds.\nWait for the doors to open."),(unsigned long)((game.lift_arrival_ms-game.round_ms+999)/1000));
            else copy(lift,sizeof(lift),tr("电梯已经离开。\n这轮还可以走楼梯。","The lift has left.\nStairs are still available."));
            snprintf(view.body,sizeof(view.body),tr("剩余 %u 层  第 %u/8 轮\n%s","%u floors left  Round %u/8\n%s"),game.floors,game.round+1,lift);
            option("等电梯：开门下两层","Lift: down 2 when open");option("走楼梯：立即下一层","Stairs: down 1 now");
            copy(view.hint,sizeof(view.hint),tr("上下选路 确定前进 长按暂停","UP/DN route  OK go  Hold pause"));break;
        }
        if(game.kind==NM_GAME_TEA){
            snprintf(view.body,sizeof(view.body),tr("第 %u/8 轮 得分 %u\n%s\n%s","Round %u/8 Score %u\n%s\n%s"),game.round+1,game.score,game.busy?tr("正在假装忙碌","Acting busy"):tr("正在喝茶","Sipping tea"),game_cue());
            copy(view.hint,sizeof(view.hint),tr("确定切换状态 长按暂停","OK switch mode  Hold pause"));break;
        }
        snprintf(view.body,sizeof(view.body),tr("第 %u/8 轮 得分 %u\n目标 %u  位置 %u\n%s","Round %u/8 Score %u\nTarget %u  Position %u\n%s"),game.round+1,game.score,game.target+1,game.lane+1,game_cue());copy(view.hint,sizeof(view.hint),tr("上下移动 确定操作 长按暂停","UP/DN move  OK act  Hold pause"));break;}
    case P_GAME_PAUSE:title("暂停","Paused");view.kind=NM_VIEW_TEXT;copy(view.body,sizeof(view.body),tr("时间已暂停。\n跳过只放弃额外奖励。","Time is paused.\nSkip forfeits extra rewards only."));option("继续游戏","Resume");option("跳过小游戏","Skip minigame");break;
    case P_STATS:title("员工资料","Employee stats");view.kind=NM_VIEW_TEXT;snprintf(view.body,sizeof(view.body),tr("饱腹 %u  精力 %u\n心情 %u  压力 %u\n健康 %u\n压力越低越好\n%s","Food %u  Energy %u\nMood %u  Stress %u\nHealth %u\nLower stress is better\n%s"),state.stat[0],state.stat[1],state.stat[2],state.stat[3],state.stat[4],state.burnout?tr("倦怠中：请休养","Burnout: take recovery"):tr("今天也照顾好自己","Take care today"));option("职业成长","Career");break;
    case P_CAREER:title("职业成长","Career");option("我的员工工牌","Employee badge");option("我的养成风格","Play style");option("季度考核与谈薪","Review & salary");option("考虑离职","Resign");option("选择新公司","New company");option("人生职业去向","Life directions");break;
    case P_BADGE:title("员工工牌","Employee badge");view.kind=NM_VIEW_SCENE;
        snprintf(view.body,sizeof(view.body),"%s | %s\n%s %u | %s %u\n%s",tr(name_zh[state.name],name_en[state.name]),tr(company_zh[state.company],company_en[state.company]),tr("职级","Rank"),state.rank+1,tr("底薪","Base"),state.company==NM_UNEMPLOYED?0:state.salary,tr(route_zh[state.route],route_en[state.route]));
        option("职业去向","Life directions");option("操作指南","Controls");option("开始今天","Start the day");break;
    case P_ROUTES:title("人生职业去向","Life directions");for(unsigned i=0;i<NM_ROUTE_COUNT;++i)option(route_zh[i],route_en[i]);break;
    case P_ROUTE:{
        static const char *zh[]={"回到普通职场路线。\n保留技能、收藏与回忆。","在职，专业 120，经验 240。\n每次工作额外经验 +6。","在职，沟通 120，前辈关系 60。\n工作后每位朋友关系 +1。","先离职，专业 96，存款 100。\n每格工作收入 12，无底薪。\n存款不扣除，无加班。","先离职，生活 96，存款 200。\n每格经营收入 10，无底薪。\n存款不扣除，无加班。","准点下班 10 次，生活 48。\n休息或散步额外心情 +5，\n压力 -5。不影响现有职位。"};
        static const char *en[]={"Return to normal office life.\nKeep skills and memories.","Employed. Tech 120, XP 240.\nWork grants +6 extra XP.","Employed. Talk 120, mentor 60.\nWork: all friendships +1.","Resign first. Tech 96, cash 100.\n12 coins per work action.\nNo base pay or overtime.\nCash is not spent.","Resign first. Life 96, cash 200.\n10 coins per work action.\nNo base pay or overtime.\nCash is not spent.","10 on-time days, Life 48.\nRest / walk: extra mood +5,\nstress -5. Keep your job."};
        title(route_zh[route_choice],route_en[route_choice]);view.kind=NM_VIEW_TEXT;
        snprintf(view.body,sizeof(view.body),"%s\n%s\n%s %lu",tr(zh[route_choice],en[route_choice]),tr("每天行动前可切换","Switch before daily actions"),tr("首次进入日","First entry day"),(unsigned long)(route_choice?state.route_day[route_choice-1]:0));
        option(nm_route_available(&state,route_choice)==NM_OK?"选择这条路线":"条件未满足",nm_route_available(&state,route_choice)==NM_OK?"Choose this direction":"Requirements not met");option("返回","Back");break;}
    case P_STYLE:{
        static const char *zh[]={"准点下班派","技术专家派","社交达人派","摸鱼大师派","努力奋斗派","生活专家派"};
        static const char *en[]={"On-time leaver","Technical expert","Social star","Break master","Striver","Life expert"};
        static const char *desc_zh[]={"工作日不加班，准点结算。","工作与专业学习，积累成长。","聊天和送礼，维系朋友。","摸鱼放松，忙里偷闲。","偶尔加班，也别忘了休息。","做饭、学习和照顾自己。"};
        static const char *desc_en[]={"Leave without overtime.","Work and study Tech.","Chat and give gifts.","Take breaks from work.","Overtime, with time to rest.","Cook, learn and care for self."};
        static const nm_scene_t scenes[]={NM_SCENE_HOME,NM_SCENE_STUDY,NM_SCENE_FRIEND,NM_SCENE_DRINK,NM_SCENE_OFFICE,NM_SCENE_COOK};
        snprintf(view.title,sizeof(view.title),tr("风格图鉴 %u/6","Style gallery %u/6"),style_choice+1);
        view.kind=NM_VIEW_SCENE;view.scene=scenes[style_choice];
        snprintf(view.body,sizeof(view.body),"%s\n%s\n%s\n%s",tr(zh[style_choice],en[style_choice]),
            style_choice==(unsigned)nm_style(&state)?tr("这是你的当前风格","Your current style"):tr("其他可养成的风格","Another possible style"),
            tr(desc_zh[style_choice],desc_en[style_choice]),tr("近 14 天行动形成，可改变。","Last 14 days; can change."));
        option("查看下一种风格","Next style");option("返回职业成长","Back to career");break;}
    case P_REVIEW:title("季度考核","Quarterly review");view.kind=NM_VIEW_TEXT;snprintf(view.body,sizeof(view.body),tr("职业经验 %u\n专业 %u 沟通 %u 生活 %u\n基本日薪 %u\n每 28 天可谈薪一次\n%s","Career XP %u\nTech %u Talk %u Life %u\nBase daily pay %u\nReview every 28 days\n%s"),state.xp,state.skill[0],state.skill[1],state.skill[2],state.salary,review_hint());option(nm_review_available(&state)?"展示成果并谈薪":"查看谈薪条件",nm_review_available(&state)?"Negotiate salary":"Review requirements");option("返回","Back");break;
    case P_RESIGN:title("下一段旅程","Next chapter");view.kind=NM_VIEW_TEXT;copy(view.body,sizeof(view.body),tr("保留积蓄、技能、朋友与收藏。\n结束当前职位和项目。\n确定要递交离职信吗？","Keep savings, skills, friends\nand collections.\nEnd the current job.\nSubmit resignation?"));option("再考虑一下","Cancel");option("确认离职","Confirm resignation");break;
    case P_LEAVING:title("收好行李，重新出发","A new chapter");view.kind=NM_VIEW_SCENE;view.scene=NM_SCENE_LEAVE;
        copy(view.body,sizeof(view.body),tr("这一段工作结束了。\n积蓄、技能和朋友都还在。\n你可以休息，也可以再出发。","This job has ended.\nSavings, skills and friends stay.\nRest, or start a new chapter."));
        option("看看新公司","Explore employers");option("先回家休息","Rest at home");break;
    case P_COMPANY:title("选择公司","Choose employer");option("稳稳公司：稳定低压","Stable: predictable");option("项目团队：奖金浮动","Projects: bonus varies");option("弹性工作室：收入浮动","Flexible: pay varies");option("先休息一阵","Rest for now");break;
    case P_SHOP:title("下班小店","After-work shop");option("工位装饰","Desk items");option("家居物品","Home items");option("服装配件","Clothing");option("我的背包","Inventory");break;
    case P_CATEGORY:case P_ITEM:{static const char *zh[]={"小绿植","猫咪台历","小风扇","新键盘","小地毯","软沙发","游戏机","小厨房","软拖鞋","大耳机","小挎包","白衬衫"};static const char *en[]={"Plant","Cat calendar","Desk fan","Keyboard","Rug","Sofa","Game console","Kitchen","Slippers","Headphones","Shoulder bag","White shirt"};
        if(page==P_CATEGORY){title("物品选择","Items");for(unsigned i=category*4;i<category*4+4;++i)option(zh[i],en[i]);}
        else{title(zh[item],en[item]);view.kind=NM_VIEW_SCENE;view.scene=item<4?NM_SCENE_OFFICE:item<8?NM_SCENE_HOME:NM_SCENE_STAND;view.preview=true;view.preview_item=item;
            snprintf(view.body,sizeof(view.body),tr("价格 %u  余额 %lu\n%s\n预览不改变当前装备。","Price %u  Cash %lu\n%s\nPreview does not equip."),nm_item_price(item),(unsigned long)state.coins,state.owned&(1u<<item)?tr("已拥有，确认装备。","Owned. Confirm to equip."):tr("购买后可装备。","Buy, then equip."));option(state.owned&(1u<<item)?"装备":"购买",state.owned&(1u<<item)?"Equip":"Buy");option("返回","Back");}break;}
    case P_BAG:title("背包","Inventory");for(unsigned i=1;i<=4;++i){char b[100];snprintf(b,sizeof(b),"%s x%u",tr(nm_actions[NM_BENTO+i-1].zh,nm_actions[NM_BENTO+i-1].en),state.food_stock[i]);option(b,b);}option("装饰与穿戴","Decorations & clothes");break;
    case P_ALBUM:title("纪念册","Memories");for(unsigned i=0;i<NM_ACHIEVEMENT_COUNT;++i){char text[100];snprintf(text,sizeof(text),"%s %s",state.achievements&(1u<<i)?"[+]":"[-]",tr(memory_zh[i],memory_en[i]));option(text,text);}break;
    case P_MEMORY:{
        static const char *story_zh[]={"钱不多，但属于自己。\n今天的努力被记住了。","认真工作，也认真生活。\n下班后的时间属于自己。","准备和成长终于被看见。\n给努力的自己一点掌声。","带着经验，选择新生活。\n离开不是把过去清零。","有积蓄，也有自己的生活。\n这不是终点，故事继续。","这次没涨，不是你不行。\n准备好后，还可以再谈。","有人愿意听你说说今天。\n关系来自一次次陪伴。","给自己做了一顿热饭。\n生活也值得认真对待。"};
        static const char *story_en[]={"Small pay, but it is yours.\nYour effort is remembered.","Work well, then live well.\nYour evening belongs to you.","Your growth was recognized.\nGive yourself some credit.","Take experience into a new life.\nLeaving does not erase the past.","Savings and a life of your own.\nNot the end. Your story goes on.","No raise does not mean no worth.\nPrepare, then try again.","Someone wants to hear your day.\nFriendship grows with care.","A warm meal made by you.\nLife deserves care, too."};
        static const char *need_zh[]={"结算一次有收入的工作日。","在工作日不加班结算十次。","成功谈薪，获得第一次涨薪。","确认一次主动离职。","金币达到 500，健康至少 70，\n准点下班二十次，睡觉结算。","完成第一次未成功的谈薪。","任一朋友关系达到 60。","生活技能达到 24 后做一次饭。"};
        static const char *need_en[]={"Settle a workday with pay.","10 workdays without overtime.","Negotiate your first pay raise.","Confirm a resignation.","Coins 500, health at least 70,\n20 on-time days; settle at sleep.","Complete a failed negotiation.","Reach bond 60 with any friend.","Reach Life 24, then cook."};
        static const nm_scene_t scenes[]={NM_SCENE_OFFICE,NM_SCENE_HOME,NM_SCENE_STAND,NM_SCENE_LEAVE,NM_SCENE_HOME,NM_SCENE_BENTO,NM_SCENE_FRIEND,NM_SCENE_COOK};
        title(memory_zh[achievement],memory_en[achievement]);view.kind=NM_VIEW_SCENE;view.scene=scenes[achievement];
        if(state.achievements&(1u<<achievement))snprintf(view.body,sizeof(view.body),tr("第 %lu 天\n%s","Day %lu\n%s"),(unsigned long)state.memory_day[achievement],tr(story_zh[achievement],story_en[achievement]));
        else snprintf(view.body,sizeof(view.body),"%s\n%s",tr("尚未解锁，慢慢来。","Not unlocked yet. Take your time."),tr(need_zh[achievement],need_en[achievement]));
        option("合上这一页","Close this page");
        if(achievement==NM_LIFE_MASTER && (state.achievements&(1u<<achievement))){option("继续自己的生活","Keep living your story");}
        break;}
    case P_SETTINGS:title("系统设置","Settings");option("声音","Sound");option("屏幕亮度","Brightness");option("自动息屏","Screen timeout");option("语言","Language");option("操作帮助","Controls");option("存档管理","Save management");option("设备状态","Device status");break;
    case P_DEVICE:{
        nm_store_status_t st=nm_storage_status();char battery[60];
        if(st.battery<0)copy(battery,sizeof(battery),tr("电量：暂不可读","Battery: unavailable"));
        else snprintf(battery,sizeof(battery),tr("电量：%d%%","Battery: %d%%"),st.battery);
        title("设备状态","Device status");view.kind=NM_VIEW_TEXT;
        snprintf(view.body,sizeof(view.body),"%s\n%s\n%s\n%s",battery,nm_audio_ready()?tr("音频：就绪","Audio: ready"):tr("音频：未就绪","Audio: unavailable"),
            st.battery>=0 && st.battery<=15?tr("电量偏低，建议连接充电。","Low battery. Please charge."):st.battery<0?tr("电量未知，不代表没电。","Unknown is not empty." ):tr("电量正常。","Battery level normal."),
            tr("无声音也能正常游玩。\n音频持续异常可保存后重启。","All games work without sound.\nIf audio stays unavailable,\nsave progress then restart."));
        option("播放测试音","Test sound");option("存档状态","Save status");option("返回","Back");break;}
    case P_SOUND:title("声音设置","Sound settings");{char b[100];snprintf(b,sizeof(b),tr("音量 %u%% %s","Volume %u%% %s"),state.volume,adjusting?"< >":"");option(b,b);option(state.key_sound?"按键音：开":"按键音：关",state.key_sound?"Key sound: on":"Key sound: off");option(state.music?"背景音乐：开":"背景音乐：关",state.music?"Music: on":"Music: off");if(adjusting)copy(view.hint,sizeof(view.hint),tr("上下调节 确定保存 长按取消","UP/DN adjust OK save Hold cancel"));}break;
    case P_BRIGHT:title("屏幕亮度","Brightness");view.kind=NM_VIEW_TEXT;snprintf(view.body,sizeof(view.body),tr("%u%%\n上下键调节，确定保存。","%u%%\nUP/DOWN adjust. OK saves."),state.brightness);option("保存亮度","Save brightness");break;
    case P_IDLE:title("自动息屏","Screen timeout");option("30 秒","30 seconds");option("60 秒","60 seconds");option("120 秒","120 seconds");option("不自动息屏","Always on");break;
    case P_LANGUAGE:title("语言","Language");option("简体中文","简体中文");option("English","English");break;
    case P_SAVE:title("存档管理","Save management");option("立即保存 / 重试","Save / retry");option("查看存档状态","Save status");option("重新开始","Start over");break;
    case P_SAVE_STATUS:{
        nm_store_status_t st=nm_storage_status();title("存档状态","Save status");view.kind=NM_VIEW_TEXT;
        const char *message;
        if(save_request_failed || st.phase==NM_STORE_ERROR)message=tr("保存失败，请重试。\n暂时不要断电。","Save failed. Please retry.\nAvoid powering off yet.");
        else if(st.pending || st.phase==NM_STORE_LOADING)message=tr("正在写入，请稍候。\n写入完成后自动更新。","Saving. Please wait.\nThis page updates automatically.");
        else if(st.phase==NM_STORE_EMPTY)message=tr("还没有已保存的角色。","No saved character yet.");
        else if(st.saved_revision!=state.revision)message=tr("有未保存的进度。\n请保存后再断电。","Unsaved progress.\nSave before powering off.");
        else message=st.recovered?tr("进度已保存。\n已从备用存档恢复。","Progress saved.\nRecovered from backup."):tr("进度已保存，可以安心退出。","Progress saved. Safe to exit.");
        copy(view.body,sizeof(view.body),message);option("保存 / 重试","Save / retry");option("返回","Back");break;}
    case P_POOR:title("零钱不太够","Not enough coins");view.kind=NM_VIEW_TEXT;
        snprintf(view.body,sizeof(view.body),tr("当前金币 %lu\n这次没有扣钱。\n可选择免费基础餐或休养。\n两者仍需一格行动。","Coins available: %lu\nNothing was charged.\nA basic meal or recovery is free.\nEach still uses one action."),(unsigned long)state.coins);
        option("免费基础餐","Free basic meal");option("免费休养","Free recovery");option("暂不购买","Back without buying");break;
    case P_EXHAUSTED:title("今天到这里","All actions used");view.kind=NM_VIEW_TEXT;
        copy(view.body,sizeof(view.body),tr("今天六格行动已经用完。\n这次没有额外消耗。\n结算后回家，睡觉进入明天。\n资料和收藏仍然可以查看。","All six actions are used.\nNothing extra was spent.\nSettle pay, go home and sleep.\nYou can still view your stats."));
        option(state.evening?"睡觉，开始明天":"查看工资条",state.evening?"Sleep until tomorrow":"View payslip");option("查看员工资料","View employee stats");option("返回","Back");break;
    case P_RESET:title("确认清除存档？","Reset your story?");view.kind=NM_VIEW_TEXT;copy(view.body,sizeof(view.body),tr("当前角色进度将被清除。\n包括金币、技能和收藏。\n此操作无法撤销。\n按住确定两秒，才会重置。","Current character progress,\ncoins, skills and collections\nwill be lost. No undo.\nHold OK for 2 seconds to reset."));option("取消","Cancel");break;
    case P_PAY:title("今日工资条","Today's payslip");view.kind=NM_VIEW_TEXT;snprintf(view.body,sizeof(view.body),tr("基本工资 %u\n绩效奖金 %u\n其他奖励 %u\n今日消费 %u\n净收益 %+ld","Base pay %u\nTarget bonus %u\nOther rewards %u\nExpenses %u\nNet %+ld"),receipt.salary,receipt.target_bonus,receipt.game_bonus,receipt.expenses,(long)receipt.net);option("回家，继续自己的生活","Go home and enjoy the evening");break;
    }
    view.selected=page==P_GAME && (game.kind==NM_GAME_MEETING || game.kind==NM_GAME_ESCAPE)?game.lane:selection;if(view.count && view.selected>=view.count)view.selected=selection=0;
}

static void finish_day(void){
    if(state.evening){nm_end_day(&state,&receipt);save();home();go(receipt.burnout_started || receipt.burnout_ended?P_BURNOUT:P_MORNING);nm_audio_play(NM_SFX_REST);}
    else{nm_close_work(&state,&receipt);save();go(P_PAY);nm_audio_play(NM_SFX_COIN);}
}
static void activate(void){
    nm_error_t err=NM_OK;
    switch(page){
    case P_BOOT:
        if(!loaded && nm_storage_status().phase==NM_STORE_ERROR){go(P_START_ERROR);break;}
        if(selection==0){if(loaded){home();if(state.burnout)go(P_BURNOUT);}else go(P_CREATE);}else if(loaded)go(P_RESET);else go(P_CREATE);break;
    case P_START_ERROR:
        if(selection==1)go(P_RESET);
        else if(nm_storage_reload()){started=false;depth=0;page=P_BOOT;selection=0;}
        break;
    case P_CREATE:if(selection==0)state.gender=!state.gender;else go(P_NAME);break;
    case P_NAME:if(selection==0)state.name=(state.name+1)%4;else{state.revision++;loaded=true;save();go(P_BADGE);}break;
    case P_HELP:home();break;
    case P_HOME:{static const page_t dest[]={P_WORK,P_FOOD,P_REST,P_SOCIAL,P_MORE};
        if(selection==0){
            if(state.evening)finish_day();
            else if(nm_weekend(&state))prepare_action(NM_EXERCISE);
            else if(state.company==NM_UNEMPLOYED && state.route!=NM_ROUTE_FREELANCE && state.route!=NM_ROUTE_SHOP)go(P_CAREER);
            else go(P_WORK);
        }else go(dest[selection]);break;}
    case P_MORE:{static const page_t dest[]={P_STATS,P_STUDY,P_CARE,P_EVENT,P_GAMES,P_SHOP,P_ALBUM,P_SETTINGS};if(selection<8)go(dest[selection]);else finish_day();break;}
    case P_WORK:if(selection==0)prepare_action(NM_WORK);else if(selection==1)prepare_game(NM_GAME_TYPING);else if(selection==2)prepare_game(NM_GAME_MEETING);else if(selection==3)prepare_action(NM_OVERTIME);else finish_day();break;
    case P_FOOD:if(selection<=NM_COOK-NM_WATER)prepare_action((nm_action_t)(NM_WATER+selection));else go(P_BAG);break;
    case P_REST:if(selection==0)prepare_action(NM_REST);else if(selection==1)prepare_action(NM_SLACK);else if(selection==2)prepare_game(NM_GAME_TEA);else if(selection==3)prepare_action(NM_RECOVER);else if(selection==4)finish_day();else{prepare_action(NM_REST);page=P_MUSIC;}break;
    case P_STUDY:if(selection==3)go(P_NOTEBOOK);else prepare_action((nm_action_t)(NM_STUDY_TECH+selection));break;
    case P_NOTEBOOK:if(selection==0)back();else go(P_CAREER);break;
    case P_CARE:{static const nm_action_t actions[]={NM_EXERCISE,NM_CLEAN,NM_TREAT,NM_RECOVER,NM_TOILET};prepare_action(actions[selection]);break;}
    case P_SOCIAL:friend_id=selection;go(P_FRIEND);break;
    case P_FRIEND:prepare_action(selection==0?NM_CHAT:NM_GIFT);break;
    case P_ACTION:case P_MUSIC:
        if(selection==1){back();break;}
        if(selection==2){food=pending_action-NM_BENTO+1;err=nm_stock_food(&state,food);if(err==NM_OK){save();result("已放入背包","Stored",tr("以后饿了再吃。","Saved for when you're hungry."),NM_SCENE_BENTO);}break;}
        {bool music=page==P_MUSIC;err=nm_act(&state,pending_action,friend_id,&last_effect);if(err==NM_OK){action_result(pending_action);if(music)page=P_MUSIC_RESULT;}}break;
    case P_RESULT:case P_EVENT_RESULT:case P_MUSIC_RESULT:home();break;
    case P_BURNOUT:if(!state.burnout || selection==2)home();else if(selection==0)prepare_action(NM_RECOVER);else go(P_SOCIAL);break;
    case P_EVENT:{
        if(state.event_done){back();break;}
        nm_state_t before=state;err=nm_event_choose(&state,selection,&last_effect);
        if(err==NM_OK){
            char text[512];snprintf(text,sizeof(text),tr("饱腹 %+d 精力 %+d\n心情 %+d 压力 %+d 健康 %+d\n金币 %+ld 消耗行动 %u\n专业 %+d 沟通 %+d 生活 %+d\n小羊 %+d 前辈 %+d 小鹿 %+d","Food %+d Energy %+d\nMood %+d Stress %+d Health %+d\nCoins %+ld Actions spent %u\nTech %+d Talk %+d Life %+d\nYang %+d Mentor %+d Lu %+d"),
                last_effect.delta[0],last_effect.delta[1],last_effect.delta[2],last_effect.delta[3],last_effect.delta[4],(long)last_effect.coins,last_effect.slots,
                (int)state.skill[0]-before.skill[0],(int)state.skill[1]-before.skill[1],(int)state.skill[2]-before.skill[2],
                (int)state.relations[0]-before.relations[0],(int)state.relations[1]-before.relations[1],(int)state.relations[2]-before.relations[2]);
            const nm_event_info_t *event=&nm_events[state.event];result(event->zh,event->en,text,NM_SCENE_FRIEND);page=P_EVENT_RESULT;save();nm_audio_play(NM_SFX_COIN);
        }else if(state.event==0){
            const char *reason=selection==2?tr("需要与小羊的关系达到 40。\n可以先聊天或送礼。","Requires Yang bond 40.\nTry chatting or a gift."):action_requirement(NM_OVERTIME,err);
            result("暂时不能执行","Not available",reason,NM_SCENE_FRIEND);nm_audio_play(NM_SFX_ERROR);err=NM_OK;
        }break;}
    case P_GAMES:prepare_game((nm_game_kind_t)selection);break;
    case P_GAME_GUIDE:
        if(selection==2)back();
        else if(selection==0)start_game(game_choice);
        else{nm_action_t a=game_action(game_choice);err=nm_act(&state,a,friend_id,&last_effect);if(err==NM_OK)action_result(a);}
        break;
    case P_GAME:break; /* key_event adjudicates click/hold at the press-time state. */
    case P_GAME_PAUSE:
        if(selection==0){back();last_tick=lv_tick_get();}
        else{nm_game_skip(&game);nm_game_settle(&game,&state);game_paid=false;save();home();}break;
    case P_STATS:go(P_CAREER);break;
    case P_CAREER:{static const page_t dest[]={P_BADGE,P_STYLE,P_REVIEW,P_RESIGN,P_COMPANY,P_ROUTES};go(dest[selection]);break;}
    case P_BADGE:if(selection==0)go(P_ROUTES);else if(selection==1)go(P_HELP);else home();break;
    case P_ROUTES:route_choice=(nm_route_t)selection;go(P_ROUTE);break;
    case P_ROUTE:if(selection==1)back();else{err=nm_choose_route(&state,route_choice);if(err==NM_OK){save();home();}}break;
    case P_STYLE:if(selection==0)style_choice=(style_choice+1)%6;else back();break;
    case P_REVIEW:
        if(selection==1){back();break;}
        if(!nm_review_available(&state)){result("谈薪条件","Review requirements",review_hint(),NM_SCENE_STAND);break;}
        {bool raised;unsigned old_salary=state.salary,old_rank=state.rank;err=nm_negotiate(&state,&raised);
            if(err==NM_OK){
                char text[320];
                if(raised)snprintf(text,sizeof(text),tr("基本日薪 %u -> %u\n职级 %u -> %u\n这次的成长被看见了。","Base pay %u -> %u\nRank %u -> %u\nYour growth was recognized."),old_salary,state.salary,old_rank+1,state.rank+1);
                else copy(text,sizeof(text),tr("这次没有涨薪，谈判经验 +10。\n准备沟通 24、职业经验 120。\n28 天后可以再试。","No raise. Negotiation XP +10.\nPrepare Talk 24 and XP 120.\nTry again in 28 days."));
                save();result(raised?"终于涨薪了":"收到画饼券",raised?"A real raise!":"An empty promise",text,NM_SCENE_STAND);
            }}break;
    case P_RESIGN:if(selection==0)back();else{err=nm_resign(&state);if(err==NM_OK){save();home();go(P_LEAVING);}}break;
    case P_LEAVING:if(selection==0)go(P_COMPANY);else home();break;
    case P_COMPANY:if(selection==3)home();else{err=nm_join(&state,(nm_company_t)selection);if(err==NM_OK){save();home();result("重新出发","Rehired",tr("技能与回忆都还在。","Your skills and memories remain."),NM_SCENE_OFFICE);}}break;
    case P_SHOP:if(selection<3){category=selection;go(P_CATEGORY);}else go(P_BAG);break;
    case P_CATEGORY:item=category*4+selection;go(P_ITEM);break;
    case P_ITEM:if(selection==1)back();else{if(state.owned&(1u<<item))err=nm_equip(&state,item);else err=nm_buy(&state,item);if(err==NM_OK){save();nm_audio_play(NM_SFX_COIN);}}break;
    case P_BAG:if(selection==4)go(P_SHOP);else{food=selection+1;err=nm_eat_stock(&state,food,&last_effect);if(err==NM_OK)action_result((nm_action_t)(NM_BENTO+selection));}break;
    case P_ALBUM:achievement=selection;go(P_MEMORY);break;
    case P_MEMORY:if(selection==1)home();else back();break;
    case P_SETTINGS:{static const page_t dest[]={P_SOUND,P_BRIGHT,P_IDLE,P_LANGUAGE,P_HELP,P_SAVE,P_DEVICE};go(dest[selection]);break;}
    case P_DEVICE:if(selection==0)nm_audio_play(NM_SFX_COIN);else if(selection==1)go(P_SAVE_STATUS);else back();break;
    case P_SOUND:
        if(selection==0 && !adjusting){adjustment_original=state.volume;adjusting=true;break;}
        if(selection==0)adjusting=false;
        else if(selection==1)state.key_sound=!state.key_sound;
        else state.music=!state.music;
        state.revision++;save();break;
    case P_BRIGHT:adjusting=false;state.revision++;save();back();break;
    case P_IDLE:{static const uint8_t times[]={30,60,120,0};state.idle_seconds=times[selection];state.revision++;save();back();break;}
    case P_LANGUAGE:state.english=selection==1;state.revision++;save();back();break;
    case P_SAVE:
        if(selection==2){go(P_RESET);break;}
        if(selection==0)save();
        go(P_SAVE_STATUS);break;
    case P_SAVE_STATUS:if(selection==0)save();else back();break;
    case P_POOR:if(selection==0)prepare_action(NM_BASIC_MEAL);else if(selection==1)prepare_action(NM_RECOVER);else back();break;
    case P_EXHAUSTED:if(selection==0)finish_day();else if(selection==1)go(P_STATS);else back();break;
    case P_MORNING:if(selection==0)home();else go(P_STATS);break;
    case P_RESET:back();break;
    case P_PAY:home();break;
    }
    if(err!=NM_OK)error(err);
}
static void key_event(key_t key){
    last_input=lv_tick_get();
    if(sleeping){
        if(key.event==BSP_BTN_CLICK){sleeping=false;bsp_display_backlight(state.brightness);}
        return; /* Consume complete wake gesture, never select a menu item. */
    }
    if(key.event==BSP_BTN_LONG && key.key==BSP_BTN_OK){
        if(page==P_RESET){uint32_t rev=state.revision+1;bool gender=state.gender;nm_init(&state,lv_tick_get(),gender,0);state.revision=rev;loaded=false;save_request_failed=!nm_storage_request(&state);depth=0;page=P_CREATE;selection=0;}
        else if(page==P_GAME){game_press_pending=false;go(P_GAME_PAUSE);}
        else if(page==P_GAME_PAUSE){back();last_tick=lv_tick_get();}
        else back();
        return;
    }
    if(page==P_GAME && key.event==BSP_BTN_PRESS){
        /* Freeze at press time until the button component distinguishes click
           from hold. A pause gesture must never spend a game attempt. */
        if(key.key==BSP_BTN_OK)game_press_pending=true;
        else if(!game_press_pending)nm_game_move(&game,key.key==BSP_BTN_UP?-1:1);
        return;
    }
    if(page==P_GAME && key.key==BSP_BTN_OK && (key.event==BSP_BTN_CLICK || key.event==BSP_BTN_DOUBLE)){
        if(game_press_pending){game_press_pending=false;bool hit=nm_game_press(&game);nm_audio_play(hit?NM_SFX_WORK:NM_SFX_ERROR);}
        return;
    }
    if(key.event!=BSP_BTN_CLICK || page==P_GAME)return;
    if(state.key_sound)nm_audio_play(NM_SFX_KEY);
    if(key.key==BSP_BTN_OK){activate();return;}
    int delta=key.key==BSP_BTN_UP?-1:1;
    if(page==P_BRIGHT || (page==P_SOUND && adjusting)){
        uint8_t *value=page==P_BRIGHT?&state.brightness:&state.volume;
        int n=(int)*value+delta*10;int min=page==P_BRIGHT?10:0;*value=n<min?min:n>100?100:n;
        if(page==P_BRIGHT)bsp_display_backlight(state.brightness);
    }else if(view.count)selection=(selection+view.count+delta)%view.count;
}
static void timer(lv_timer_t *t){
    (void)t;
    uint32_t now=lv_tick_get(),delta=now-last_tick;last_tick=now;
    nm_store_status_t store=nm_storage_status();
    if(!started){
        if(store.phase==NM_STORE_LOADING){
            key_t ignored;while(xQueueReceive(keys,&ignored,0)==pdTRUE){} /* Never replay loading gestures. */
            describe();nm_view_draw(&view,&state,&game,store,now/400);return;
        }
        loaded=nm_storage_loaded(&state);started=true;
        if(!loaded)nm_init(&state,now,true,0);
        if(!loaded && store.phase==NM_STORE_ERROR){page=P_START_ERROR;selection=0;depth=0;}
        bsp_display_backlight(state.brightness);last_input=now;
    }
    key_t key;while(xQueueReceive(keys,&key,0)==pdTRUE){key_event(key);describe();}
    if(!sleeping && state.idle_seconds && now-last_input>state.idle_seconds*1000u){sleeping=true;bsp_display_backlight(0);save();}
    /* Lost release/hold events must not freeze the game indefinitely. */
    if(game_press_pending && now-last_input>3000)game_press_pending=false;
    if(!sleeping && page==P_GAME && !game_press_pending){
        nm_game_tick(&game,delta);
        nm_state_t before=state;
        if(game.done && game_paid && nm_game_settle(&game,&state)){
            game_paid=false;save();char text[200];snprintf(text,sizeof(text),tr("得分 %u/8\n基础收益保留\n额外金币 +%u","Score %u/8\nBase reward kept\nExtra coins +%u"),game.score,game.score);
            result("小游戏完成","Game complete",text,NM_SCENE_STAND);nm_audio_play(NM_SFX_COIN);
            if(game.kind==NM_GAME_MEETING){
                snprintf(result_body,sizeof(result_body),tr("得分 %u/8\n额外金币 +%lu\n经验 +%u 小羊关系 +%u\n沟通 +%u 精力 %+d\n基础学习收益保留","Score %u/8\nExtra coins +%lu\nXP +%u Yang bond +%u\nTalk +%u Energy %+d\nBase study reward kept"),game.score,(unsigned long)(state.coins-before.coins),state.xp-before.xp,state.relations[0]-before.relations[0],state.skill[1]-before.skill[1],(int)state.stat[NM_ENERGY]-before.stat[NM_ENERGY]);
                page=P_EVENT_RESULT;
            }
            if(game.kind==NM_GAME_ESCAPE){
                snprintf(result_body,sizeof(result_body),tr("%s\n下楼 %u/3 层\n额外金币 +%lu\n基础散步收益保留","%s\nDescended %u/3 floors\nExtra coins +%lu\nBase walk reward kept"),game.floors?tr("今天的路线有点绕。","A winding route today."):tr("走出大门，下班啦！","Outside at last!"),3-game.floors,(unsigned long)(state.coins-before.coins));
                page=P_EVENT_RESULT;
            }
        }
    }
    nm_audio_config(state.volume,state.music || page==P_MUSIC_RESULT,sleeping);
    if(!sleeping && now-last_draw>=80){describe();nm_view_draw(&view,&state,&game,store,now/400);last_draw=now;frame++;}
}
bool niuma_app_start(void){
    keys=xQueueCreate(24,sizeof(key_t));if(!keys)return false;
    nm_init(&state,1,true,0);
    if(!nm_view_init()){vQueueDelete(keys);keys=NULL;return false;}
    describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);
    last_tick=last_input=lv_tick_get();
    return lv_timer_create(timer,20,NULL)!=NULL;
}
void niuma_app_key(bsp_btn_t button,bsp_btn_ev_t event,void *user){
    (void)user;if(keys){key_t key={button,event};xQueueSend(keys,&key,0);}
}
