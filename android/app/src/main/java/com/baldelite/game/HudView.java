package com.baldelite.game;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.RectF;
import android.view.MotionEvent;
import android.view.View;
import java.util.ArrayList;

public final class HudView extends View {
    private final GameState game;
    private final Paint p=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final ArrayList<Control> buttons=new ArrayList<>();
    private boolean orbit=false,handledButton=false;
    private float lastX,lastY;
    private static final int GOLD=0xffedc477,PALE=0xfff4e9dc,MUTED=0xffa9c1cb,
        NAVY=0xee0a1b29,MINT=0xff60e6b9;
    private static final class Control {
        final String label,action;
        final RectF area;
        Control(String text,String command,float x0,float y0,float x1,float y1){
            label=text;action=command;area=new RectF(x0,y0,x1,y1);
        }
    }
    public HudView(Context context,GameState state){super(context);game=state;setLayerType(View.LAYER_TYPE_SOFTWARE,null);}
    private void color(int c){p.setColor(c);p.setShader(null);p.setStyle(Paint.Style.FILL);}
    private void box(Canvas c,float l,float t,float r,float b,int bg,float radius){
        color(bg);c.drawRoundRect(new RectF(l,t,r,b),radius,radius,p);
    }
    private void text(Canvas c,String s,float x,float y,int size,int tint,boolean center){
        color(tint);p.setTextSize(size);p.setTypeface(android.graphics.Typeface.create("sans-serif-medium",android.graphics.Typeface.NORMAL));
        p.setTextAlign(center?Paint.Align.CENTER:Paint.Align.LEFT);
        c.drawText(s,x,y,p);
    }
    private void button(Canvas c,String label,String action,int l,int t,int r,int b,boolean primary){
        buttons.add(new Control(label,action,l,t,r,b));
        box(c,l,t,r,b,primary?0xffbf9650:0xe922394b,7);
        box(c,l+1,t+1,r-1,b-2,primary?0xffdfb76b:0xff30485a,6);
        text(c,label,(l+r)/2f,(t+b)/2f+4,11,primary?0xff132130:PALE,true);
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        canvas.save();
        canvas.scale(getWidth()/480f,getHeight()/270f);
        buttons.clear();
        synchronized(game){
            box(canvas,8,8,472,45,0xcc0a1b29,12);
            text(canvas,"BALD ELITE",20,32,22,GOLD,false);
            text(canvas,"ANDROID EDITION",169,31,11,MUTED,false);
            if(game.phase==GameState.TITLE){
                box(canvas,22,56,205,178,0xd80b2331,11);
                text(canvas,"CHOOSE APPLICANT",35,78,12,GOLD,false);
                text(canvas,game.candidate+1+" / 3",35,99,18,PALE,false);
                text(canvas,GameState.NAMES[game.candidate],35,121,12,PALE,false);
                text(canvas,GameState.COUNTS[game.candidate]+" HAIRS TO SHAVE",35,140,11,MUTED,false);
                text(canvas,"BEST: "+game.best,35,161,12,MINT,false);
                box(canvas,240,59,462,96,0xb60b2331,9);
                text(canvas,game.mode==GameState.ZEN?"ZEN MODE - NO TIMER":"CHALLENGE - 90 SECONDS",351,82,13,GOLD,true);
                box(canvas,19,191,461,214,0xd0102938,6);
                text(canvas,"SHAVE WITH ONE FINGER  |  TWO FINGERS TO ROTATE",240,207,12,MUTED,true);
                button(canvas,"PREV","PREV",9,224,86,259,false);
                button(canvas,"NEXT","NEXT",92,224,170,259,false);
                button(canvas,"ZEN / 90S","MODE",175,224,292,259,false);
                button(canvas,"SOUND","SOUND",297,224,372,259,false);
                button(canvas,"PLAY","PLAY",377,224,474,259,true);
            }else if(game.phase==GameState.PLAY){
                float fraction=(float)game.removed/Math.max(1,game.hairs.length);
                box(canvas,10,49,470,78,0xc3081c29,7);
                box(canvas,20,56,320,68,0xff3b5361,3);
                box(canvas,20,56,20+300*fraction,68,MINT,3);
                text(canvas,game.percentage()+"% SHAVED",329,67,11,PALE,false);
                box(canvas,10,83,157,107,0xb70c2130,6);
                text(canvas,(game.mode==GameState.ZEN?"TIME ":"LEFT ")+format(game.timeLeft()),19,100,13,GOLD,false);
                box(canvas,163,83,333,107,0xb70c2130,6);
                text(canvas,"SCORE "+game.score+"  COMBO "+game.combo,174,100,12,PALE,false);
                box(canvas,339,83,470,107,0xb70c2130,6);
                text(canvas,game.hairs.length-game.removed+" HAIRS LEFT",349,100,12,MINT,false);
                if(game.paused){
                    box(canvas,117,117,365,194,0xed091a2a,14);
                    text(canvas,"PAUSED",241,151,25,GOLD,true);
                    text(canvas,"TAP PAUSE TO RESUME",241,176,12,PALE,true);
                }else if(game.cursorX>=0){
                    float x=game.cursorX*480f/Math.max(1,getWidth());
                    float y=game.cursorY*270f/Math.max(1,getHeight());
                    p.setStyle(Paint.Style.STROKE);p.setStrokeWidth(1.8f);p.setColor(GOLD);
                    canvas.drawCircle(x,y,13,p);p.setStyle(Paint.Style.FILL);
                    color(MINT);canvas.drawCircle(x,y,2.5f,p);
                }
                box(canvas,8,220,472,265,0xe60b2030,9);
                button(canvas,"TURN L","LEFT",13,226,65,259,false);
                button(canvas,"TURN R","RIGHT",69,226,121,259,false);
                button(canvas,"UP","UP",125,226,164,259,false);
                button(canvas,"DOWN","DOWN",168,226,217,259,false);
                button(canvas,"ZOOM","ZOOM",221,226,275,259,false);
                button(canvas,"HINT","HINT",279,226,326,259,false);
                button(canvas,game.paused?"RESUME":"PAUSE","PAUSE",330,226,394,259,false);
                button(canvas,"MENU","MENU",398,226,467,259,false);
            }else{
                box(canvas,99,55,381,196,0xed0a1d2d,14);
                if(game.phase==GameState.WIN){
                    text(canvas,"MEMBERSHIP APPROVED!",240,87,19,GOLD,true);
                    text(canvas,"BALD BOSS  "+(game.stars()==3?"***":game.stars()==2?"**":"*"),240,116,18,MINT,true);
                    text(canvas,"SCORE "+game.score+"   BEST "+game.best,240,144,15,PALE,true);
                    text(canvas,"ANOTHER BALD BOSS!",240,173,13,MUTED,true);
                }else{
                    text(canvas,"TIME IS UP!",240,87,24,GOLD,true);
                    text(canvas,game.percentage()+"% SHAVED",240,119,19,PALE,true);
                    text(canvas,"SCORE "+game.score,240,148,15,MUTED,true);
                }
                button(canvas,"RETRY","PLAY",60,221,187,261,true);
                if(game.phase==GameState.WIN)button(canvas,"NEXT","NEXTPLAY",194,221,321,261,false);
                button(canvas,"MENU","MENU",329,221,454,261,false);
            }
        }
        canvas.restore();
        postInvalidateDelayed(33);
    }
    private static String format(float secs){
        int s=(int)Math.max(0,secs);
        return String.format(java.util.Locale.US,"%02d:%02d",s/60,s%60);
    }
    private void click(String action){
        switch(action){
            case "PREV":game.select(game.candidate-1);break;
            case "NEXT":game.select(game.candidate+1);break;
            case "MODE":game.setMode(game.mode==GameState.ZEN?GameState.CHALLENGE:GameState.ZEN);break;
            case "SOUND":game.toggleSound();break;
            case "PLAY":game.start();break;
            case "NEXTPLAY":game.next();break;
            case "LEFT":game.turn(-.24f);break;
            case "RIGHT":game.turn(.24f);break;
            case "UP":game.tilt(.20f);break;
            case "DOWN":game.tilt(-.20f);break;
            case "ZOOM":game.changeZoom();break;
            case "HINT":game.hintLast(getWidth(),getHeight());break;
            case "PAUSE":game.paused=!game.paused;game.endShave();break;
            case "MENU":game.menu();break;
        }
        invalidate();
    }
    @Override public boolean onTouchEvent(MotionEvent e){
        int action=e.getActionMasked();
        float scaleX=480f/Math.max(1,getWidth()),scaleY=270f/Math.max(1,getHeight());
        if(action==MotionEvent.ACTION_DOWN){
            handledButton=false;orbit=false;
            float x=e.getX()*scaleX,y=e.getY()*scaleY;
            synchronized(game){
                for(Control b:buttons){
                    if(b.area.contains(x,y)){
                        handledButton=true;click(b.action);
                        return true;
                    }
                }
                if(game.phase==GameState.PLAY&&!game.paused){
                    game.beginShave(e.getX(),e.getY());
                    // One frame of touch contact is already a valid shave.
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
