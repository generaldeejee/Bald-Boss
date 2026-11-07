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
                panel(c,75,191,405,228,0xd112102a,8);
                text(c,"AN INVITATION TO THE BALD ELITE",240,208,13,GOLD,true);
                text(c,"Only the perfectly polished shall enter.",240,222,9,WHITE,true);
                button(c,"ENTER THE CLUB","ENTER",145,233,335,265,true);
                float t=SystemClock.uptimeMillis()*.0027f;
                color(0x77f6ca7a);
                p.setStrokeWidth(2.1f);p.setStyle(Paint.Style.STROKE);
                c.drawRoundRect(new RectF(143,231,337,267),8,8,p);
                p.setStyle(Paint.Style.FILL);
                if(t>0){} // The spotlight animation runs via periodic redraw.
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
                text(c,"PREVIEW APPLICANT",388,82,11,GOLD,true);
                panel(c,13,207,467,222,0xe21b1331,5);
                text(c,"DRAG TO SHAVE  •  TWO FINGERS TO TURN  •  HD GRAPHICS",240,218,10,WHITE,true);
                button(c,"PREV","PREV",9,229,65,262,false);
                button(c,"NEXT","NEXT",70,229,122,262,false);
                button(c,game.mode==GameState.ZEN?"90 SEC":"ZEN","MODE",127,229,205,262,false);
                button(c,game.sound?"MUSIC ON":"MUSIC OFF","SOUND",210,229,291,262,false);
                button(c,game.highDetail?"HD ON":"HD OFF","HD",296,229,374,262,false);
                button(c,"PLAY","PLAY",379,229,473,262,true);
            }else if(game.phase==GameState.PLAY){
                panel(c,6,6,474,42,0xd0140e29,9);
                text(c,"THE ELITE BALD MAN CLUB",17,25,15,GOLD,false);
                text(c,game.highDetail?"HD DELUXE":"STANDARD",363,25,11,PURPLE,false);
                panel(c,6,47,474,80,0xc0101b2a,7);
                float fraction=(float)game.removed/Math.max(1,game.hairs.length);
                panel(c,14,55,306,68,0xff3a4256,3);
                panel(c,14,55,14+292*fraction,68,MINT,3);
                text(c,game.percentage()+"% SHAVED",320,67,12,WHITE,false);
                panel(c,6,84,149,108,0xbe101829,6);
                text(c,(game.mode==GameState.ZEN?"TIME ":"LEFT ")+format(game.timeLeft()),16,101,13,GOLD,false);
                panel(c,154,84,332,108,0xbe101829,6);
                text(c,"SCORE "+game.score+"  COMBO "+game.combo,165,101,12,WHITE,false);
                panel(c,337,84,474,108,0xbe101829,6);
                text(c,(game.hairs.length-game.removed)+" HAIRS LEFT",347,101,12,MINT,false);
                if(game.paused){
                    panel(c,110,113,372,193,0xf015102b,14);
                    text(c,"CLUB SESSION PAUSED",241,146,21,GOLD,true);
                    text(c,"TAP RESUME TO CONTINUE",241,172,12,WHITE,true);
                }else if(game.cursorX>=0){
                    float x=game.cursorX*480f/Math.max(1,getWidth());
                    float y=game.cursorY*270f/Math.max(1,getHeight());
                    color(GOLD);p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(1.8f);
                    c.drawCircle(x,y,12,p);
                    color(MINT);c.drawCircle(x,y,2.5f,p);
                }
                panel(c,5,222,475,266,0xdd1a132f,9);
                button(c,"TURN L","LEFT",10,227,65,259,false);
                button(c,"TURN R","RIGHT",69,227,124,259,false);
                button(c,"UP","UP",128,227,166,259,false);
                button(c,"DOWN","DOWN",170,227,220,259,false);
                button(c,"ZOOM","ZOOM",224,227,279,259,false);
                button(c,"HINT","HINT",283,227,330,259,false);
                button(c,game.paused?"RESUME":"PAUSE","PAUSE",334,227,394,259,false);
                button(c,"MENU","MENU",398,227,471,259,false);
            }else if(game.phase==GameState.WIN){
                art(c,victoryArt);
                sparkles(c);
                panel(c,102,174,380,226,0xd9151029,10);
                text(c,"YOUR CLUB RECORD",241,191,10,GOLD,true);
                text(c,"SCORE "+game.score+"   BEST "+game.best,241,210,16,WHITE,true);
                button(c,"RETRY","PLAY",45,232,162,265,false);
                button(c,"NEXT APPLICANT","NEXTPLAY",171,232,309,265,true);
                button(c,"CLUB MENU","MENU",319,232,451,265,false);
            }else{
                art(c,victoryArt);
                panel(c,93,63,387,213,0xf0190e28,12);
                text(c,"MEMBERSHIP DENIED",240,102,23,GOLD,true);
                text(c,"THE CLOCK HAS RUN OUT",240,133,13,WHITE,true);
                text(c,"SHAVED "+game.percentage()+"%  •  SCORE "+game.score,240,168,16,MINT,true);
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
                    game.enterClub();handledButton=true;return true;
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
