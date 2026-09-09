/* Production-controller journey. Only the initial character fixture is set
 * directly; all progress, purchases and careers use queued button clicks. */
static void journey_click(bsp_btn_t key){
    niuma_app_key(key,BSP_BTN_CLICK,NULL);advance(20);
}
static void journey_pick(unsigned index){
    describe();assert(index<view.count);
    for(unsigned guard=0;selection!=index;++guard){assert(guard<12);journey_click(BSP_BTN_DOWN);}
    journey_click(BSP_BTN_OK);
}
static void journey_more(unsigned index){
    assert(page==P_HOME);journey_pick(4);assert(page==P_MORE);journey_pick(index);
}
static void journey_career(void){
    journey_more(0);assert(page==P_STATS);journey_pick(0);assert(page==P_CAREER);
}
static void journey_saved(void){assert(nm_valid(&state));assert(!memcmp(&state,&saved_state,sizeof(state)));}
static void journey_action(nm_action_t action,unsigned friend_index){
    assert(page==P_HOME);
    if(action>=NM_WATER){journey_pick(1);assert(page==P_FOOD);journey_pick(action-NM_WATER);}
    else if(action>=NM_STUDY_TECH && action<=NM_STUDY_LIFE){journey_more(1);assert(page==P_STUDY);journey_pick(action-NM_STUDY_TECH);}
    else if(action==NM_CHAT){journey_pick(3);journey_pick(friend_index);journey_pick(0);}
    else if(action==NM_WORK){journey_pick(0);assert(page==P_WORK);journey_pick(0);}
    else{assert(action==NM_RECOVER || action==NM_REST);journey_pick(2);journey_pick(action==NM_REST?0:3);}
    assert(page==P_ACTION && pending_action==action);journey_pick(0);
    assert(page==P_RESULT);journey_saved();journey_pick(0);assert(page==P_HOME);
}
static void journey_finish(void){
    uint32_t day=state.day;journey_more(8);assert(page==P_PAY && state.evening);journey_saved();
    journey_pick(0);assert(page==P_HOME);journey_pick(0);
    assert(page==P_MORNING && state.day==day+1);journey_saved();journey_pick(0);assert(page==P_HOME);
}
static void journey_route(nm_route_t route){
    journey_career();journey_pick(5);assert(page==P_ROUTES);journey_pick(route);
    assert(page==P_ROUTE);journey_pick(0);assert(page==P_HOME && state.route==route);journey_saved();
}
static void journey_resign(void){
    journey_career();journey_pick(3);assert(page==P_RESIGN);journey_pick(1);
    assert(page==P_LEAVING && state.company==NM_UNEMPLOYED);journey_saved();journey_pick(1);assert(page==P_HOME);
}
static void full_controller_career(void){
    for(unsigned lang=0;lang<2;++lang)for(unsigned gender=0;gender<2;++gender){
        nm_init(&state,41,gender,0);state.english=lang;loaded=false;started=true;depth=0;page=P_BOOT;selection=0;
        journey_pick(0);assert(page==P_CREATE);journey_pick(1);assert(page==P_NAME);
        journey_pick(1);assert(page==P_BADGE && loaded);journey_pick(2);assert(page==P_HOME);journey_saved();
        bool cooked=false;
        while(state.day<=56){
            if(state.day==28 || state.day==56){
                journey_career();journey_pick(2);assert(page==P_REVIEW);journey_pick(0);
                assert(page==P_RESULT && state.salary==(state.day==28?20:24));journey_saved();journey_pick(0);
            }
            journey_action(NM_BASIC_MEAL,0);
            if(!cooked && state.skill[2]>=24){journey_action(NM_COOK,0);cooked=true;}
            else journey_action(NM_BASIC_MEAL,0);
            journey_action(nm_weekend(&state)?NM_CHAT:NM_WORK,1);
            journey_action((nm_action_t)(NM_STUDY_TECH+(state.day-1)%3),0);
            journey_action(NM_RECOVER,0);
            journey_action(state.skill[2]<96?NM_STUDY_LIFE:state.relations[1]<60?NM_CHAT:state.skill[0]<120?NM_STUDY_TECH:NM_STUDY_TALK,1);
            assert(!state.actions);journey_finish();
        }
        for(unsigned product=0;product<12;++product){
            journey_more(5);assert(page==P_SHOP);journey_pick(product/4);journey_pick(product%4);assert(page==P_ITEM);
            uint8_t equipped=state.equipped[product/4];journey_pick(0);
            assert(page==P_ITEM && (state.owned&(1u<<product)) && state.equipped[product/4]==equipped);journey_saved();
            journey_pick(0);assert(state.equipped[product/4]==product);journey_saved();
            for(unsigned guard=0;page!=P_HOME;++guard){assert(guard<16);niuma_app_key(BSP_BTN_OK,BSP_BTN_LONG,NULL);advance(20);}
        }
        journey_route(NM_ROUTE_EXPERT);journey_action(NM_WORK,0);assert(last_effect.experience==18);journey_finish();
        journey_route(NM_ROUTE_MANAGER);unsigned bond=state.relations[0];journey_action(NM_WORK,0);assert(state.relations[0]==bond+1);journey_finish();
        journey_resign();
        for(unsigned route=NM_ROUTE_FREELANCE;route<=NM_ROUTE_SHOP;++route){
            journey_route((nm_route_t)route);uint32_t coins=state.coins;journey_action(NM_WORK,0);journey_finish();
            assert(state.coins==coins+(route==NM_ROUTE_FREELANCE?12:10));
        }
        journey_route(NM_ROUTE_COMFORT);journey_action(NM_REST,0);journey_finish();
        for(unsigned employer=0;employer<NM_UNEMPLOYED;++employer){
            if(state.company!=NM_UNEMPLOYED)journey_resign();
            uint32_t coins=state.coins;uint16_t xp=state.xp;
            journey_career();journey_pick(4);assert(page==P_COMPANY);journey_pick(employer);
            assert(page==P_RESULT && state.company==employer && state.coins==coins && state.xp==xp);journey_saved();journey_pick(0);
        }
        assert(state.owned==0xfff && state.achievements==((1u<<NM_ACHIEVEMENT_COUNT)-1));
        for(unsigned i=0;i<NM_ACHIEVEMENT_COUNT;++i)assert(state.memory_day[i] && state.memory_day[i]<=state.day);
        for(unsigned i=0;i<NM_ROUTE_COUNT-1;++i)assert(state.route_day[i] && state.route_day[i]<=state.day);
        printf("Controller career: language %u gender %u day %lu; all routes, items, memories PASS\n",lang,gender,(unsigned long)state.day);fflush(stdout);
    }
}
