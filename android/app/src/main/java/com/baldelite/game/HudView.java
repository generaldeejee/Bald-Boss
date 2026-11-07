package com.baldelite.game;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.LinearGradient;
import android.graphics.Shader;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Locale;
import java.util.Random;

/**
 * Deluxe club front-end. Existing artwork is drawn at true 16:9 resolution
 * in front of the OpenGL scene. All actions remain real touch targets.
 * Artists may replace assets/club_*.jpg with the full cinematic images.
 */
public final class HudView extends View {
    private final GameState game;
    private final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG|Paint.FILTER_BITMAP_FLAG);
    private final ArrayList<Control> buttons=new ArrayList<>();
    private Bitmap introArt, victoryArt, crest;
    private boolean orbit=false,handledButton=false;
    private float lastX,lastY;
    private static final int GOLD=0xfff2c978,WHITE=0xfffff7eb,MUTED=0xffb5c2c9,
            NAVY=0xee0d1224,MINT=0xff69e9c1,PURPLE=0xffb68dee;
    private static final class Control{
        final String label,action;
        final RectF area;
        Control(String s,String a,float l,float t,float r,float b){
            label=s;action=a;area=new RectF(l,t,r,b);
        }
    }
    public HudView(Context ctx,GameState state){
        super(ctx);game=state;
        // Only three fixed UI textures: never decode on the drawing thread.
        introArt=load("club_intro.jpg");
        victoryArt=load("club_victory.jpg");
        crest=load("club_crest.png");
    }
    private Bitmap load(String filename){
        try(InputStream input=getContext().getAssets().open(filename)){
            return BitmapFactory.decodeStream(input);
        }catch(Exception ignored){return null;}
    }
    private void color(int col){
        p.setColor(col);p.setStyle(Paint.Style.FILL);p.setShader(null);
    }
    private void panel(Canvas c,float l,float t,float r,float b,int bg,float rad){
        color(bg);c.drawRoundRect(new RectF(l,t,r,b),rad,rad,p);
    }
    private void text(Canvas c,String msg,float x,float y,float size,int tint,boolean center){
        color(tint);
        p.setTextSize(size);
        p.setTypeface(android.graphics.Typeface.create("sans-serif-medium",android.graphics.Typeface.NORMAL));
        p.setTextAlign(center?Paint.Align.CENTER:Paint.Align.LEFT);
        c.drawText(msg,x,y,p);
    }
    private void art(Canvas c,Bitmap image){
        if(image!=null){
            color(0xffffffff);
            RectF dst=new RectF(0,0,480,270);
            int w=image.getWidth(),h=image.getHeight();
            float target=480f/270f;
            int cropW=w,cropH=h;
            if(w/(float)h>target)cropW=(int)(h*target);
            else cropH=(int)(w/target);
            android.graphics.Rect crop=new android.graphics.Rect((w-cropW)/2,(h-cropH)/2,(w+cropW)/2,(h+cropH)/2);
            c.drawBitmap(image,crop,dst,p);
        }else{
            p.setShader(new LinearGradient(0,0,480,270,0xff120e26,0xff34204d,Shader.TileMode.CLAMP));
            c.drawRect(0,0,480,270,p);
            p.setShader(null);
            text(c,"BALD ELITE",240,90,42,GOLD,true);
            text(c,"THE ELITE BALD MAN CLUB",240,117,16,WHITE,true);
        }
    }
    private void button(Canvas c,String label,String action,float l,float t,float r,float b,boolean primary){
        buttons.add(new Control(label,action,l,t,r,b));
        panel(c,l,t+2,r,b+2,0xaa070713,6);
        panel(c,l,t,r,b,primary?0xffd0a150:0xdd25223f,6);
        panel(c,l+1,t+1,r-1,b-2,primary?0xfff5d488:0xff44355e,5);
        text(c,label,(l+r)*.5f,(t+b)*.5f+4,10.4f,primary?0xff271a18:WHITE,true);
    }
    private void sparkles(Canvas c){
        float time=SystemClock.uptimeMillis()*.001f;
        color(GOLD);
        for(int i=0;i<35;i++){
            float x=(i*93+57)%480;
            float y=((i*47+time*(15+(i%4)*9))%290)-10;
            p.setColor(i%2==0?0xfff2ca6e:0xfffff3cd);
            c.save();c.rotate((time*45+i*27)%360,x,y);
            c.drawRect(x-1.3f,y-2.6f,x+1.3f,y+2.6f,p);
            c.restore();
        }
    }
    @Override protected void onDraw(Canvas c){
        super.onDraw(c);
        c.save();
        c.scale(getWidth()/480f,getHeight()/270f);
        buttons.clear();
        synchronized(game){
            if(game.phase==GameState.INTRO){
                art(c,introArt);
                if(game.introPage==0){
                    panel(c,28,153,452,230,0xe51c1029,11);
                    text(c,"WELCOME TO THE BALD MAN CLUB",240,177,20,GOLD,true);
                    text(c,"An exclusive brotherhood of shiny gentlemen.",240,195,11,WHITE,true);
                    text(c,"ONLY THE PERFECTLY BALD MAY ENTER.",240,216,13,MINT,true);
                    button(c,"READ THE CLUB RULES","INTRO_NEXT",130,235,350,266,true);
                }else{
                    panel(c,29,54,451,231,0xf0180f28,12);
                    text(c,"THE COUNCIL'S FOUR GOLDEN RULES",240,78,17,GOLD,true);
                    text(c,"1. Only bald men may join the inner circle.",50,103,12,WHITE,false);
                    text(c,"2. Every last strand must go.",50,126,12,WHITE,false);
                    text(c,"3. A polished scalp is a badge of honour.",50,149,12,WHITE,false);
                    text(c,"4. Combs are strictly forbidden.",50,172,12,WHITE,false);
                    text(c,"No hair. No fear. No refunds on shampoo.",240,209,11,MINT,true);
                    button(c,"BACK","INTRO_BACK",42,237,167,266,false);
                    button(c,"ACCEPT & ENTER CLUB","ENTER",187,237,439,266,true);
                }
            }else if(game.phase==GameState.TITLE){
                panel(c,5,5,474,46,0xc7151029,8);
                if(crest!=null){
                    color(0xffffffff);c.drawBitmap(crest,null,new RectF(10,2,60,50),p);
                }
                text(c,"BALD ELITE",65,25,20,GOLD,false);
                text(c,"THE ELITE BALD MAN CLUB",66,39,9,WHITE,false);
                panel(c,13,60,218,203,0xe6161029,9);
                text(c,"CLUB INITIATION",26,81,14,GOLD,false);
                text(c,"APPLICANT  "+(game.candidate+1)+"/3",26,103,13,WHITE,false);
                text(c,GameState.NAMES[game.candidate],26,122,13,GOLD,false);
                text(c,game.hairs.length+" STRANDS TO REMOVE",26,140,11,MUTED,false);
                text(c,game.mode==GameState.ZEN?"ZEN / NO TIMER":"CHALLENGE / 90 SECONDS",26,159,10,WHITE,false);
                text(c,"PERSONAL BEST: "+game.best,26,181,11,MINT,false);
                panel(c,314,65,463,90,0xbb1a1030,8);
                text(c,"BALD CANDIDATE",388,82,11,GOLD,true);
                panel(c,303,170,467,204,0xd01c122d,7);
                button(c,"ABOUT THE CLUB","RULES",310,174,461,200,false);
                panel(c,13,207,467,222,0xe21b1331,5);
                text(c,"CLUB MOTTO:  "+game.motto(),240,218,10,WHITE,true);
                button(c,"PREV","PREV",9,229,65,262,false);
                button(c,"NEXT","NEXT",70,229,122,262,false);
                button(c,game.mode==GameState.ZEN?"90 SEC":"ZEN","MODE",127,229,205,262,false);
                button(c,game.sound?"MUSIC ON":"MUSIC OFF","SOUND",210,229,291,262,false);
                button(c,game.highDetail?"HD ON":"HD OFF","HD",296,229,374,262,false);
                button(c,"PLAY","PLAY",379,229,473,262,true);
                if(game.aboutOpen){
                    buttons.clear();
                    panel(c,22,35,458,243,0xf6160e29,12);
                    text(c,"THE BALD MAN CLUB",240,62,23,GOLD,true);
                    text(c,"An invitation-only society for polished gentlemen.",240,82,11,WHITE,true);
                    text(c,"MEMBERSHIP REQUIREMENTS",42,108,13,GOLD,false);
                    text(c,"• No visible hair. Absolutely no exceptions.",42,129,12,WHITE,false);
                    text(c,"• Our barbers inspect every follicle.",42,148,12,WHITE,false);
                    text(c,"• Baldness is not a loss. It is an upgrade.",42,167,12,WHITE,false);
                    text(c,"• The council demands a dazzling dome.",42,186,12,WHITE,false);
                    text(c,"NOTICE: Unauthorized combs will be confiscated.",240,216,11,MINT,true);
                    button(c,"BACK TO APPLICANTS","RULES",126,236,354,267,true);
                }
            }else if(game.phase==GameState.PLAY){
                // v2.3: total UI occlusion shrunk to 38px top and 29px bottom.
                // The entire central game field is now exposed for grooming.
                panel(c,4,3,476,31,0xb10b1322,6);
                text(c,(game.mode==GameState.ZEN?"ZEN ":"TIME ")+format(game.timeLeft()),
                     12,21,13,GOLD,false);
                text(c,"SCORE "+game.score+"  /  x"+game.combo,170,21,12,WHITE,true);
                text(c,game.percentage()+"%  •  "+(game.hairs.length-game.removed)+" HAIRS",
                     463,21,12,MINT,true);
                // Five-pixel gold and mint progress track, not a floating panel.
                float fraction=(float)game.removed/Math.max(1,game.hairs.length);
                panel(c,12,34,468,38,0x99535b6c,2);
                if(fraction>0)panel(c,12,34,12+456*fraction,38,MINT,2);
                // The head occupies the unobstructed y=39..239 play window.
                if(game.paused){
                    panel(c,112,106,368,181,0xe9120e29,11);
                    text(c,"SESSION PAUSED",240,137,20,GOLD,true);
                    text(c,"TAP RESUME TO CONTINUE",240,157,11,WHITE,true);
                }else if(game.cursorX>=0){
                    float x=game.cursorX*480f/Math.max(1,getWidth());
                    float y=game.cursorY*270f/Math.max(1,getHeight());
                    color(GOLD);
                    p.setStyle(Paint.Style.STROKE);
                    p.setStrokeWidth(1.5f);
                    c.drawCircle(x,y,10,p);
                    color(MINT);
                    c.drawCircle(x,y,2.0f,p);
                }
                // Semi-transparent one-line controller dock. Gesture shaving
                // and two-finger orbit work across all of the empty center.
                panel(c,4,241,476,269,0xc01b1630,5);
                button(c,"TURN L","LEFT",6,244,62,266,false);
                button(c,"TURN R","RIGHT",64,244,121,266,false);
                button(c,"UP","UP",123,244,165,266,false);
                button(c,"DOWN","DOWN",167,244,215,266,false);
                button(c,"ZOOM","ZOOM",217,244,276,266,false);
                button(c,"HINT","HINT",278,244,322,266,false);
                button(c,game.paused?"RESUME":"PAUSE","PAUSE",324,244,397,266,false);
                button(c,"MENU","MENU",399,244,474,266,false);
            }else if(game.phase==GameState.WIN){
                art(c,victoryArt);
                sparkles(c);
                panel(c,49,159,431,228,0xe7151029,10);
                text(c,"MEMBERSHIP GRANTED",240,175,13,GOLD,true);
                text(c,game.resultLine(),240,195,11,WHITE,true);
                text(c,"SCORE "+game.score+"   BEST "+game.best,240,216,15,MINT,true);
                button(c,"RETRY","PLAY",45,232,162,265,false);
                button(c,"NEXT APPLICANT","NEXTPLAY",171,232,309,265,true);
                button(c,"CLUB MENU","MENU",319,232,451,265,false);
            }else{
                art(c,victoryArt);
                panel(c,93,63,387,213,0xf0190e28,12);
                text(c,"MEMBERSHIP DENIED",240,102,23,GOLD,true);
                text(c,"THE CLOCK HAS RUN OUT",240,128,13,WHITE,true);
                text(c,"SHAVED "+game.percentage()+"%  •  SCORE "+game.score,240,156,16,MINT,true);
                text(c,game.resultLine(),240,185,12,GOLD,true);
                button(c,"RETRY","PLAY",70,229,224,263,true);
                button(c,"CLUB MENU","MENU",254,229,409,263,false);
            }
        }
        c.restore();
        postInvalidateDelayed(33);
    }
    private static String format(float seconds){
        int s=(int)Math.max(0,seconds);
        return String.format(Locale.US,"%02d:%02d",s/60,s%60);
    }
    private void click(String action){
        switch(action){
            case "INTRO_NEXT":game.nextIntro();break;
            case "INTRO_BACK":game.prevIntro();break;
            case "RULES":game.toggleAbout();break;
            case "ENTER":game.enterClub();break;
            case "PREV":game.select(game.candidate-1);break;
            case "NEXT":game.select(game.candidate+1);break;
            case "MODE":game.setMode(game.mode==GameState.ZEN?GameState.CHALLENGE:GameState.ZEN);break;
            case "SOUND":game.toggleSound();break;
            case "HD":game.toggleQuality();break;
            case "PLAY":game.start();break;
            case "NEXTPLAY":game.next();break;
            case "LEFT":game.turn(-.24f);game.uiSerial++;break;
            case "RIGHT":game.turn(.24f);game.uiSerial++;break;
            case "UP":game.tilt(.20f);game.uiSerial++;break;
            case "DOWN":game.tilt(-.20f);game.uiSerial++;break;
            case "ZOOM":game.changeZoom();game.uiSerial++;break;
            case "HINT":game.hintLast(getWidth(),getHeight());game.uiSerial++;break;
            case "PAUSE":game.paused=!game.paused;game.endShave();game.uiSerial++;break;
            case "MENU":game.menu();break;
        }
        invalidate();
    }
    @Override public boolean onTouchEvent(MotionEvent e){
        int action=e.getActionMasked();
        float sx=480f/Math.max(1,getWidth()),sy=270f/Math.max(1,getHeight());
        if(action==MotionEvent.ACTION_DOWN){
            handledButton=false;orbit=false;
            float x=e.getX()*sx,y=e.getY()*sy;
            synchronized(game){
                for(Control b:buttons)if(b.area.contains(x,y)){
                    handledButton=true;click(b.action);return true;
                }
                if(game.phase==GameState.INTRO){
                    game.nextIntro();handledButton=true;return true;
                }
                if(game.phase==GameState.PLAY&&!game.paused){
                    game.beginShave(e.getX(),e.getY());
                    game.shaveAt(e.getX(),e.getY(),getWidth(),getHeight());
                }
            }
            lastX=e.getX();lastY=e.getY();
            return true;
        }
        if(action==MotionEvent.ACTION_POINTER_DOWN){
            synchronized(game){game.endShave();}
            orbit=true;
            if(e.getPointerCount()>1){
                lastX=(e.getX(0)+e.getX(1))*.5f;
                lastY=(e.getY(0)+e.getY(1))*.5f;
            }
            return true;
        }
        if(action==MotionEvent.ACTION_MOVE&&!handledButton){
            synchronized(game){
                if(orbit&&e.getPointerCount()>1){
                    float x=(e.getX(0)+e.getX(1))*.5f,y=(e.getY(0)+e.getY(1))*.5f;
                    game.turn((x-lastX)*.012f);game.tilt(-(y-lastY)*.012f);
                    lastX=x;lastY=y;
                }else if(game.phase==GameState.PLAY&&!game.paused){
                    game.dragShave(e.getX(),e.getY(),getWidth(),getHeight());
                }
            }
        }
        if(action==MotionEvent.ACTION_UP||action==MotionEvent.ACTION_CANCEL){
            synchronized(game){game.endShave();}
            orbit=false;handledButton=false;
        }
        if(action==MotionEvent.ACTION_POINTER_UP){
            synchronized(game){game.endShave();}
        }
        return true;
    }
}
