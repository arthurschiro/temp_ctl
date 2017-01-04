function serial_data_logger()
close all
clear all
clc
delete(instrfind);
delete(timerfindall());

% port='COM4';
port='COM16';

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
      
edit_view_dur_hr=uicontrol(fig,'style','edit',...
                          'callback',@(src,event)edit_view_dur_hr_cb(src,event,fig),...
                          'units','normalized','position',[.95 .55 .05 .2]);
                        
edit_view_dur_min=uicontrol(fig,'style','edit',...
                          'callback',@(src,event)edit_view_dur_min_cb(src,event,fig),...
                          'units','normalized','position',[.95 .3 .05 .2]);

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
                     
                        
s = serial(port,...
           'baudrate',9600,...
           'terminator','CR/LF',...
           'BytesAvailableFcn',@(src,event)serial_cb(src,event,fig));
  
plot_period=.1;
timer_obj=timer('executionmode','fixedrate',...
                'period',plot_period,...
                'timerfcn',@(src,event)timer_cb(src,event,fig));

ts=.1;
disp_long_resample=10;
disp_dur_long=60*60*2;
disp_dur_short=5;
disp_dur_mid=60*5;

buff_dur=60*60*10;
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
handles.buff_dur=buff_dur;
handles.data_buff=zeros(buff_len,3);
handles.data_size_is_calibrated=false;
handles.data_buff_ptr=1;
handles.timer_obj=timer_obj;
         
guidata(fig,handles);
fopen(s);
start(timer_obj);


end

function serial_cb(src,event,fig)
  handles=guidata(fig);
  
  line=str2num(fgetl(handles.s))/100;
  
  
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
end

function timer_cb(src,event,fig)
  handles=guidata(fig);
  
  if handles.data_buff_ptr==1
    newest_ptr=handles.buff_len;
  else
    newest_ptr=handles.data_buff_ptr-1;
  end
  
    
  mid_len=round(handles.disp_dur_mid*handles.fs);
  idx=newest_ptr-((mid_len-1):-1:0);
  idx(idx<=0)=idx(idx<=0)+handles.buff_len;
  plot_data_mid=handles.data_buff(idx,:);
  
  short_len=round(handles.disp_dur_short*handles.fs);
  plot_data_short=plot_data_mid(end-short_len+1:end,:);
      
  
  long_len=round(handles.disp_dur_long*handles.fs/handles.disp_long_resample);
  idx=newest_ptr-(((long_len-1):-1:0)*handles.disp_long_resample);
  idx(idx<=0)=idx(idx<=0)+handles.buff_len;
  plot_data_long=handles.data_buff(idx,:);
  
  ax=handles.ax_short;
  grid(ax,'on');
  plot(ax,plot_data_short);
  xlabel(ax,sprintf('%.0f sec',handles.disp_dur_short));
  
  ax=handles.ax_mid;
  grid(ax,'on');
  maxval=ceil(max(plot_data_mid(:)));
  minval=floor(min(plot_data_mid(:)));
  plot(ax,plot_data_mid);
  set(ax,'ylim',[minval,max(minval+1,maxval)]);
  xlabel(ax,sprintf('%.1f min',handles.disp_dur_mid/60));
  
  ax=handles.ax_long;
  grid(ax,'on');
  maxval=ceil(max(plot_data_mid(:)));
  minval=floor(min(plot_data_mid(:)));
  plot(ax,plot_data_long);
  set(ax,'ylim',[minval,max(minval+1,maxval)]);
  xlabel(ax,sprintf('%.1f hr',handles.disp_dur_long/60/60));
  title(ax,sprintf('temp=%.2f  setpoint=%.2f',plot_data_mid(end,1),plot_data_mid(end,2)));
  
  guidata(fig,handles);
  drawnow();
end

function button_quit_cb(src,event,fig)
  quit(fig);
end

function button_save_cb(src,event,fig)
  handles=guidata(fig);
  
  if handles.data_buff_ptr==1
    newest_ptr=handles.buff_len;
  else
    newest_ptr=handles.data_buff_ptr-1;
  end
  
  save_struct=[];
  save_struct.data=[handles.data_buff(handles.data_buff_ptr:end,:);...
                    handles.data_buff(1:newest_ptr,:)];
  save_struct.ts=handles.ts;

  filename=datestr(now,'yyyy_mm_dd__HH_MM_SS_PM');
  save(filename,'save_struct');
end

function edit_view_dur_hr_cb(src,event,fig)
  handles=guidata(fig);
  
  dur=min(handles.buff_dur,str2num(get(src,'string'))*60*60);
  handles.disp_dur_long=dur;
  
  guidata(fig,handles);
end

function edit_view_dur_min_cb(src,event,fig)
  handles=guidata(fig);
  
  dur=min(handles.buff_dur,str2num(get(src,'string'))*60);
  handles.disp_dur_mid=dur;
  
  guidata(fig,handles);
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