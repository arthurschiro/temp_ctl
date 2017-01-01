function serial_data_logger()
close all
clear all
clc
delete(instrfind);
delete(timerfindall());

fig=figure;
set(fig,'buttondownfcn',@(src,event)fig_cb(src,event,fig));

ax_short=axes('parent',fig,...
        'xtick',[],...
        'nextplot','replacechildren',...
        'units','normalized','position',[.1 .05 .85 .2]);
      
ax_mid=axes('parent',fig,...
        'xtick',[],...
        'nextplot','replacechildren',...
        'units','normalized','position',[.1 .3 .85 .2]);
      
ax_long=axes('parent',fig,...
        'xtick',[],...
        'nextplot','replacechildren',...
        'units','normalized','position',[.1 .55 .85 .2]);

button_quit=uicontrol(fig,'style','pushbutton','string','QUIT',...
                          'callback',@(src,event)button_quit_cb(src,event,fig),...
                          'units','normalized','position',[0 .95 .2 .05]);
                        
button_save=uicontrol(fig,'style','pushbutton','string','SAVE',...
                          'callback',@(src,event)button_save_cb(src,event,fig),...
                          'units','normalized','position',[.2 .95 .2 .05]);

edit_temp=uicontrol(fig,'style','edit',...
                          'callback',@(src,event)edit_temp_cb(src,event,fig),...
                          'units','normalized','position',[0 .9 .5 .05]);
                        
edit_hyst=uicontrol(fig,'style','edit',...
                          'callback',@(src,event)edit_hyst_cb(src,event,fig),...
                          'units','normalized','position',[.5 .9 .5 .05]);
                     
                        
s = serial('COM4',...
           'baudrate',9600,...
           'terminator','CR/LF',...
           'BytesAvailableFcn',@(src,event)serial_cb(src,event,fig));
  
plot_period=.1;
timer_obj=timer('executionmode','fixedrate',...
                'period',plot_period,...
                'timerfcn',@(src,event)timer_cb(src,event,fig));

ts=.1;
disp_long_resample=10;
disp_dur_long=60*30;
disp_dur_short=5;
disp_dur_mid=60*5;

buff_dur=60*60;
buff_len=round(buff_dur/ts);


handles=[];
handles.s=s;
handles.fig=fig;
handles.ax_short=ax_short;
handles.ax_mid=ax_mid;
handles.ax_long=ax_long;
handles.ts=ts;
handles.fs=1/ts;
handles.disp_dur_short=disp_dur_short;
handles.disp_dur_mid=disp_dur_mid;
handles.disp_dur_long=disp_dur_long;
handles.disp_long_resample=disp_long_resample;
handles.buff_len=buff_len;
handles.data_buff=randn(buff_len,1);
handles.continue=true;
handles.data_size_is_calibrated=false;
handles.data_buff_ptr=1;
handles.timer_obj=timer_obj;
         
guidata(fig,handles);
fopen(s);
start(timer_obj);


end

function serial_cb(src,event,fig)
  handles=guidata(fig);
  
  line=str2num(fgetl(handles.s));
  
  
  if handles.data_size_is_calibrated==false
    handles.data_buff=repmat(line,handles.buff_len,1);
    handles.data_size_is_calibrated=true;
  else
    handles.data_buff(handles.data_buff_ptr,:)=line;
    
    if handles.data_buff_ptr==handles.buff_len
      handles.data_buff_ptr=1;
    else
      handles.data_buff_ptr=handles.data_buff_ptr+1;
    end
  end
  
  guidata(fig,handles);
  drawnow();
end

function timer_cb(src,event,fig)
  handles=guidata(fig);
%   tic
  if handles.data_buff_ptr==1
    plot_data=handles.data_buff;    
  else
    plot_data=[handles.data_buff(handles.data_buff_ptr:end,:);...
               handles.data_buff(1:handles.data_buff_ptr-1,:)];
  end
%   disp(toc);
  
  short_len=round(handles.disp_dur_short*handles.fs);
  plot_data_short=plot_data(end-short_len+1:end,:);
  
  mid_len=round(handles.disp_dur_mid*handles.fs);
  plot_data_mid=plot_data(end-mid_len+1:end,:);
  
  long_len=round(handles.disp_dur_long*handles.fs);
  plot_data_long=plot_data(end-long_len+1:handles.disp_long_resample:end,:);
  
  ax=handles.ax_short;
  grid(ax,'on');
  plot(ax,plot_data_short);
  
  ax=handles.ax_mid;
  grid(ax,'on');
  maxval=ceil(max(plot_data_mid(:))/100)*100;
  minval=floor(min(plot_data_mid(:))/100)*100;
  plot(ax,plot_data_mid);
  set(ax,'ylim',[minval,max(minval+1,maxval)]);
  
  ax=handles.ax_long;
  grid(ax,'on');
  maxval=ceil(max(plot_data_mid(:))/100)*100;
  minval=floor(min(plot_data_mid(:))/100)*100;
  plot(ax,plot_data_long);
  set(ax,'ylim',[minval,max(minval+1,maxval)]);
  
  guidata(fig,handles);
  drawnow();
end

function button_quit_cb(src,event,fig)
  quit(fig);
end

function button_save_cb(src,event,fig)
  handles=guidata(fig);
  filename=datestr(now,'yyyy_mm_dd__HH_MM_PM');
  save(filename,handles);
end

function edit_temp_cb(src,event,fig)
  handles=guidata(fig);
  
  cmd=round(str2num(get(src,'string'))*1000);
  
  fprintf(handles.s,'temp %u\n',cmd);
  
end

function edit_hyst_cb(src,event,fig)
  handles=guidata(fig);
  
  cmd=round(str2num(get(src,'string'))*1000);
  
  fprintf(handles.s,'hyst %u\n',cmd);
  
end

function fig_cb(src,event,fig)
end

function ax_cb(src,event,fig)
end

function quit(fig)
  drawnow();
  handles=guidata(fig);
  stop(handles.timer_obj)
  delete(handles.timer_obj)
  clear handles.timer_obj;
  fclose(handles.s);
  delete(handles.s)
  clear handles.s
  close(handles.fig);
  disp('done');
end