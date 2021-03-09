postdata = load('air30325a.out'); %første søjle: GPS-tid efter midnat, følgeligt lat long h
imudata = dlmread('20120325_imu.pos','',1,0);
utcoffset=16;
% difflon = postdata(:,3)-imudata(:,3);
% difflat = postdata(:,2)-imudata(:,2);
% diffh = postdata(:,4)-imudata(:,4);

% t = postdata(:,1);
% t2=datenum(datestr(seconds(t),'HH:MM'));
% 
% 
% postdata(:,1)=postdata(:,1)-utcoffset;
% tiledlayout('flow');
% nexttile
% plot(postdata(:,1),postdata(:,4));
% hold on 
% plot(imudata(:,1),imudata(:,4));
% title(['height']);
% hold off
% nexttile
% plot(postdata(:,1),postdata(:,3));
% hold on 
% title(['Longitude']);
% plot(imudata(:,1),imudata(:,3));
% hold off
% nexttile
% plot(postdata(:,1),postdata(:,2));
% hold on 
% title(['Latitude']);
% plot(imudata(:,1),imudata(:,2));
% hold off
% 
wmline(postdata(:,2),postdata(:,3));


imu_table = array2table(imudata,'VariableNames',{'TimeOfDay(UTC)','PosLat(deg)','PosLon(deg)','PosHeight(m)','AngleRoll(deg)','AnglePitch(deg)','Heading(deg)'});
stackedplot(imu_table);
% tiledlayout('flow')
% plot(imudata(:,1),imudata(:,5);
% hold on
wmline(imudata(:,2),imudata(:,3));
c = 1;
for i=1:1000:length(imudata)-1000
    description = sprintf('<br> Time: %0.5g <br> Angle Roll: %0.5g <br> Angle Pitch: %0.5g <br> Heading(deg): %0.5g' ,imudata(i,1),imudata(i,5),imudata(i,6),imudata(i,7)); 
    pointticks = sprintf('Pointtick: point %0.5g',c);
%     wmmarker(imudata(i,2),imudata(i,3));
    wmmarker(imudata(i,2),imudata(i,3),'Description',description,'Color',[1-(i/length(imudata)),i/(length(imudata)),i/(2*length(imudata))],'OverlayName',pointticks);
%     wmmarker(p,'FeatureName',p.Name,'OverlayName','Boston Placenames')
    i
    c = c + 1;
end
