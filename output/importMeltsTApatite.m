 %% Import saturation data
if ~exist('igncn1','var'); load igncn1; end
name='tApatite10F06kbar';
load([name '.log']);

% Make struct from input file 


variables={'Kv','Mbulk','Tliq','TsatApb','Tf','TsatAp','P2O5sat','P2O5f','Ff','SiO2','P2O5b','MAp'};
tapatitelog=struct;
for i=1:length(variables)
   eval(['tapatitelog.(variables{i})=' name '(:,i);'])
end

% Create new struct fields for imported data
variables={'Mbulk','Tliq','TsatAp','TsatApb','P2O5sat','P2O5f','Ff','MAp'};
for var=variables;
    igncn1.(var{:})=NaN(size(igncn1.Kv));
end
for var=variables;
    igncn1.err.(var{:})=0.02;
end

% Parse melts struct row by row, inserting each value in the right place
for i=1:length(igncn1.Kv)
    j=find(tapatitelog.Kv==igncn1.Kv(i));
    if length(j)==1
        for var=variables;
            igncn1.(var{:})(i)=tapatitelog.(var{:})(j);
        end
    elseif length(j)>1
        printf('Warning: Duplicate sample number')
    end
end
    
    
    
%%
t=igncn1.MAp>0;
figure; plot(igncn1.SiO2(~t), igncn1.MAp(~t),'.r');
hold on; plot(igncn1.SiO2(t), igncn1.MAp(t),'.b');
xlabel('Bulk SiO2 (%)'); ylabel('Mass of apatite saturated (ug/g)');
formatfigure

t=igncn1.MAp>0;
figure; plot(igncn1.P2O5(~t), igncn1.MAp(~t),'.r');
hold on; plot(igncn1.P2O5(t), igncn1.MAp(t),'.b');
xlabel('Bulk P2O5 (ppm)'); ylabel('Mass of apatite saturated (ug/g)');
formatfigure

t=igncn1.MAp>0;
figure; plot(igncn1.TsatApb(~t), igncn1.MAp(~t),'.r');
hold on; plot(igncn1.TsatApb(t), igncn1.MAp(t),'.b');
xlabel('Bulk apatite saturation temp (C)'); ylabel('Mass of apatite saturated (ug/g)');
formatfigure

t=igncn1.MAp>0;
figure; plot(igncn1.TsatApb(~t), igncn1.TsatAp(~t),'.r');
hold on; plot(igncn1.TsatApb(t), igncn1.TsatAp(t),'.b');
xlabel('Bulk apatite saturation temp (C)'); ylabel('Adjusted apatite saturation temp (C)');
formatfigure

t=igncn1.MAp>0;
figure; plot(igncn1.TsatAp(~t), igncn1.MAp(~t),'.r');
hold on; plot(igncn1.TsatAp(t), igncn1.MAp(t),'.b');
xlabel('Adjusted apatite saturation temp (C)'); ylabel('Mass of apatite saturated (ug/g)');
formatfigure

t=igncn1.MAp>0;
figure; plot(igncn1.Tliq(~t), igncn1.TsatAp(~t),'.r');
hold on; plot(igncn1.Tliq(t), igncn1.TsatAp(t),'.b');
xlabel('Liquidus temperature (C)'); ylabel('Adjusted apatite saturation temp (C)');
formatfigure

t=igncn1.MAp>0;
figure; plot(igncn1.Tliq(~t), igncn1.TsatApb(~t),'.r');
hold on; plot(igncn1.Tliq(t), igncn1.TsatApb(t),'.b');
xlabel('Liquidus temperature (C)'); ylabel('Bulk apatite saturation temp (C)');
formatfigure

t=igncn1.MAp>0;
figure; plot(igncn1.SiO2(~t), igncn1.TsatApb(~t),'.r');
hold on; plot(igncn1.SiO2(t), igncn1.TsatApb(t),'.b');
xlabel('Bulk SiO2 (%)'); ylabel('Bulk apatite saturation temp (C)');
formatfigure

t=igncn1.MAp>0;
figure; plot(igncn1.SiO2(~t), igncn1.TsatAp(~t),'.r');
hold on; plot(igncn1.SiO2(t), igncn1.TsatAp(t),'.b');
xlabel('Bulk SiO2 (%)'); ylabel('Adjusted apatite saturation temp (C)');
formatfigure
