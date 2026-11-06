package com.baldelite.game;

import android.content.SharedPreferences;
import java.util.Random;

public final class GameState {
    public static final int TITLE=0, PLAY=1, WIN=2, FAIL=3;
    public static final int CHALLENGE=0, ZEN=1;
    public static final String[] NAMES={"THE BUSINESSMAN","THE PUNK","THE PROFESSOR"};
    public static final int[] COUNTS={900,1250,760};
    private final SharedPreferences prefs;
    public int phase=TITLE, candidate=0, mode=CHALLENGE, zoom=0;
    public boolean paused=false, sound=true;
    public float yaw=0, pitch=0, seconds=0, cursorX=-1, cursorY=-1;
    public int score=0, combo=0, removed=0, best=0, total=0, lastHits=0;
    public float comboTimer=0;
    public Hair[] hairs=new Hair[0];
    public boolean hairDirty=true;
    public boolean showHelp=true;
    private float strokeX,strokeY;
    private boolean stroke=false;
    private boolean bonus=false;

    public static final class Hair {
        public float x,y,z,nx,ny,nz,len;
        public boolean gone;
    }
    public GameState(SharedPreferences preferences){
        prefs=preferences;
        sound=prefs.getBoolean("sound",true);
        select(0);
    }
    public void select(int c){
        candidate=(c+3)%3;
        yaw=pitch=0;zoom=0;paused=false;phase=TITLE;
        score=combo=removed=lastHits=0;seconds=0;stroke=false;bonus=false;
        regenerate();loadBest();
    }
    private void loadBest(){best=prefs.getInt("best_"+candidate+"_"+mode,0);}
    public void setMode(int m){mode=m;loadBest();}
    public void toggleSound(){sound=!sound;prefs.edit().putBoolean("sound",sound).apply();}
    public void start(){
        phase=PLAY;paused=false;score=0;combo=0;removed=0;lastHits=0;seconds=0;
        yaw=pitch=0;zoom=0;stroke=false;comboTimer=0;bonus=false;
        regenerate();loadBest();
    }
    public void menu(){select(candidate);}
    public void next(){candidate=(candidate+1)%3;start();}
    public int percentage(){return hairs.length==0?0:(removed*100/hairs.length);}
    public int stars(){return mode==ZEN||seconds<=50?3:(seconds<=75?2:1);}
    public float timeLeft(){return mode==ZEN?seconds:Math.max(0,90-seconds);}
    public float distance(){return zoom==0?3.8f:(zoom==1?3.1f:2.65f);}
    public void update(float dt){
        if(phase!=PLAY||paused)return;
        dt=Math.max(0,Math.min(dt,.12f));
        seconds+=dt;
        comboTimer+=dt;
        if(comboTimer>1f)combo=0;
        if(mode==CHALLENGE&&seconds>=90&&removed<hairs.length){
            phase=FAIL;stroke=false;
        }
    }
    public void beginShave(float x,float y){
        if(phase!=PLAY||paused)return;
        stroke=true;strokeX=x;strokeY=y;cursorX=x;cursorY=y;
        shaveSweep(x,y,x,y,0,0);
    }
    public void dragShave(float x,float y,int width,int height){
        if(!stroke||phase!=PLAY||paused)return;
        shaveSweep(strokeX,strokeY,x,y,width,height);
        strokeX=x;strokeY=y;cursorX=x;cursorY=y;
    }
    public void endShave(){stroke=false;cursorX=-1;cursorY=-1;}
    public void turn(float diff){yaw+=diff;if(yaw>Math.PI*2)yaw-=(float)(Math.PI*2);if(yaw< -Math.PI*2)yaw+=(float)(Math.PI*2);}
    public void tilt(float diff){pitch=Math.max(-.72f,Math.min(.78f,pitch+diff));}
    public void changeZoom(){zoom=(zoom+1)%3;}
    private void win(){
        phase=WIN;paused=false;stroke=false;total++;
        if(score>best){best=score;prefs.edit().putInt("best_"+candidate+"_"+mode,best).apply();}
    }
    private static float smooth(float a,float b,float t){float u=t*t*(3-2*t);return a+(b-a)*u;}
    private static final float[] W={.025f,.44f,.60f,.66f,.65f,.63f,.59f,.43f,.12f};
    private static final float[] D={.035f,.40f,.62f,.66f,.67f,.65f,.63f,.50f,.20f};
    private static final float[] C={-.045f,-.05f,-.06f,-.05f,-.04f,-.02f,.02f,.12f,.27f};
    private static final float[] Y={1.14f,.98f,.75f,.45f,.18f,-.18f,-.48f,-.79f,-1.02f};
    private static float interpolate(float y,float[] arr){
        if(y>=Y[0])return arr[0];
        for(int i=0;i<8;i++)if(y>=Y[i+1])return smooth(arr[i],arr[i+1],(Y[i]-y)/(Y[i]-Y[i+1]));
        return arr[8];
    }
    private static float gauss(float x){return (float)Math.exp(-x*x);}
    public static float[] headPoint(float y,float a){
        float sa=(float)Math.sin(a),ca=(float)Math.cos(a);
        float x=interpolate(y,W)*sa;
        float z=interpolate(y,C)+interpolate(y,D)*ca;
        float bridge=gauss(a/.20f)*gauss((y+.04f)/.29f);
        float tip=gauss(a/.16f)*gauss((y+.25f)/.12f);
        float wing=(gauss((a-.16f)/.085f)+gauss((a+.16f)/.085f))*gauss((y+.31f)/.09f);
        z+=.078f*bridge+.168f*tip+.035f*wing;
        float eyes=gauss((a-.40f)/.18f)+gauss((a+.40f)/.18f);
        z-=.043f*eyes*gauss((y-.25f)/.125f);
        z+=.019f*eyes*gauss((y-.38f)/.09f);
        z+=.026f*eyes*gauss((y+.08f)/.20f);
        float mid=gauss(a/.34f);
        z+=mid*(.022f*gauss((y+.53f)/.050f)+.034f*gauss((y+.62f)/.060f)+.035f*gauss((y+.80f)/.125f));
        float ear=(gauss((a-1.58f)/.22f)+gauss((a+1.58f)/.22f))*gauss((y+.11f)/.18f);
        x+=sa*.095f*ear;
        if(ca>0)z+=ca*.012f*ear;
        return new float[]{x,y,z};
    }
    private void regenerate(){
        Random rand=new Random(0xB01D2026L+candidate*7319L);
        hairs=new Hair[COUNTS[candidate]];
        int count=0,attempt=0;
        while(count<hairs.length&&attempt<hairs.length*100){
            attempt++;
            float y=.34f+.79f*rand.nextFloat();
            float a=(rand.nextFloat()-.5f)*(float)(2*Math.PI);
            float[] p=headPoint(y,a);
            if(candidate==1&&Math.abs(p[0])>.28f)continue;
            if(candidate==2){
                if(y>.86f&&Math.abs(p[0])<.58f)continue;
                if(p[2]>.27f&&y>.57f&&Math.abs(p[0])<.52f)continue;
                if(rand.nextFloat()<.24f)continue;
            }
            Hair h=new Hair();
            h.x=p[0];h.y=y;h.z=p[2];
            float nx=(float)Math.sin(a),nz=(float)Math.cos(a),ny=Math.max(.10f,(y-.2f)*.40f);
            float inverse=(float)(1.0/Math.sqrt(nx*nx+ny*ny+nz*nz));
            h.nx=nx*inverse;h.ny=ny*inverse;h.nz=nz*inverse;
            h.len=candidate==0?.026f+.020f*rand.nextFloat():(candidate==1?.115f+.075f*rand.nextFloat():.032f+.035f*rand.nextFloat());
            hairs[count++]=h;
        }
        if(count<hairs.length){
            Hair[] actual=new Hair[count];System.arraycopy(hairs,0,actual,0,count);hairs=actual;
        }
        removed=0;hairDirty=true;
    }
    public int shaveSweep(float x0,float y0,float x1,float y1,int width,int height){
        if(phase!=PLAY||paused||width<=0||height<=0)return 0;
        float cy=(float)Math.cos(yaw),sy=(float)Math.sin(yaw);
        float cp=(float)Math.cos(pitch),sp=(float)Math.sin(pitch);
        float f=(float)(height/(2*Math.tan(Math.toRadians(25))));
        float camera=distance();
        float vx=x1-x0,vy=y1-y0;
        float len2=vx*vx+vy*vy;
        float radius=Math.max(12f,height*.055f);
        float r2=radius*radius;
        int count=0;
        for(Hair h:hairs){
            if(h.gone)continue;
            float xx=cy*h.x+sy*h.z;
            float zz=cy*h.z-sy*h.x;
            float yy=cp*h.y-sp*zz,zr=sp*h.y+cp*zz;
            float nz=cy*h.nz-sy*h.nx;
            float facing=sp*h.ny+cp*nz;
            if(facing<-.06f||camera-zr<.8f)continue;
            float scale=f/(camera-zr);
            float sx=width*.5f+xx*scale, py=height*.5f-(yy-.06f)*scale;
            float q=len2>1f?Math.max(0,Math.min(1,((sx-x0)*vx+(py-y0)*vy)/len2)):0;
            float dx=sx-x0-q*vx,dy=py-y0-q*vy;
            if(dx*dx+dy*dy>r2)continue;
            h.gone=true;removed++;count++;
        }
        if(count>0){
            hairDirty=true;
            if(comboTimer>.5f)combo=0;
            combo=Math.min(100,combo+count);
            comboTimer=0;
            lastHits=count;
            score+=count*20+Math.min(60,combo/3);
            if(removed>=hairs.length)win();
            else if(!bonus && mode==CHALLENGE&&hairs.length-removed<=10){
                bonus=true;
                seconds=Math.max(0,seconds-20);
            }
        }
        return count;
    }
    public void shaveAt(float x,float y,int w,int h){shaveSweep(x,y,x,y,w,h);}
    public void hintLast(int w,int h){
        if(phase!=PLAY||hairs.length-removed>8)return;
        for(Hair h:hairs)if(!h.gone){
            turn((float)Math.atan2(h.nx,h.nz)-yaw);
            h.gone=true;removed++;score+=20;hairDirty=true;
            if(removed==hairs.length)win();
            break;
        }
    }
}
