#include "mdaio.h"
#include <QFile>
#include <QList>
#include <QMap>
#include <QProcess>
#include <stdio.h>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>

struct JOB {
    QList<int> channels;
    QString input_path;
    QString output_path;
    long file_pos;
    QProcess *process;
};

void launch_worker(JOB *X,int TT) {
    X->process=new QProcess();
    QStringList args; args
            << "worker"
            << X->input_path
            << X->output_path
            << QString("%1").arg(X->file_pos)
            << QString("%1").arg(X->channels.count())
            << QString("%1").arg(TT);
    X->process->start(qApp->applicationFilePath(),args);
    X->process->waitForStarted();
    X->process->waitForFinished();
}

bool all_jobs_complete(QList<JOB> &jobs) {
    for (int jj=0; jj<jobs.count(); jj++) {
        if (jobs[jj].process) {
            if (jobs[jj].process->state()==QProcess::Running) {
                return false;
            }
            else {
                if (jj==0) {
                    qDebug() << QString(jobs[jj].process->readAll());
                }
                delete jobs[jj].process;
                jobs[jj].process=0;
            }
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    QStringList input_args;
    for (int j=1; j<argc; j++) input_args << QString(argv[j]);
    if (input_args.value(0)=="controller") {
        QCoreApplication app(argc,argv);

        QString input_path=input_args.value(1);

        JJobManager JM;
        {
            QStringList args; args << "reader" << input_path;
            JJob *J=new JJob(app.applicationFilePath(),args);
            JM.startJob("controller",J);
            while (J->isRunning()) {
                app.processEvents();
                if (J->hasMessage()) {
                    QMap<QString,QVariant> msg=take_message();

                }
            }
        }

        return 0;
    }
}

int main_old(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);

    QStringList input_args;
    for (int j=1; j<argc; j++) input_args << QString(argv[j]);
    if (input_args.value(0)=="worker") {
        QString input_path=input_args.value(1);
        QString output_path=input_args.value(2);
        int file_pos=input_args.value(3).toInt();
        int num_channels=input_args.value(4).toInt();
        int TT=input_args.value(5).toInt();
        FILE *inf=fopen(input_path.toLatin1().data(),"r");
        FILE *outf=fopen(output_path.toLatin1().data(),"a");
        fseek(inf,file_pos,SEEK_SET);
        float *buf=(float *)malloc(sizeof(float)*num_channels*TT);
        fread(buf,sizeof(float),num_channels*TT,inf);
        printf("Writing %d bytes",sizeof(float)*num_channels*TT);
        fwrite(buf,sizeof(float),num_channels*TT,outf);
        free(buf);
        fclose(inf);
        fclose(outf);
        return 0;
    }
    else {
        QDir("/tmp").mkdir("rtfilt");
        FILE *inf=fopen("/home/magland/data/EJ/Spikes_all_channels_filtered.mda","r");
        MDAIO_HEADER HH;
        mda_read_header(&HH,inf);
        int num_channels=HH.dims[0];
        int num_timepoints=HH.dims[1];
        printf("datatype=%d\n",HH.data_type);

        QString rand_id=QString("X_%1").arg(qrand()%100000);

        int num_jobs=qMin(10,num_channels);
        QList<JOB> jobs;
        int incr=num_channels/num_jobs;
        for (int i=0; i<num_jobs; i++) {
            JOB X;
            for (int ch=incr*i; (ch<incr*(i+1))&&(ch<num_channels); ch++) {
                X.channels << ch;
            }
            X.input_path=QString("/tmp/rtfilt/%1-%2.dat").arg(rand_id).arg(i);
            X.output_path=QString("/tmp/rtfilt/%1-%2-out.dat").arg(rand_id).arg(i);
            X.file_pos=0;
            X.process=0;
            if (QFile::exists(X.input_path)) QFile::remove(X.input_path);
            if (QFile::exists(X.output_path)) QFile::remove(X.output_path);
            jobs << X;
        }

        int timepoint=0;
        int TT=100;
        while ((timepoint<1000)&&(timepoint+TT-1<num_timepoints)) {
            if (all_jobs_complete(jobs)) {
                float *buf=(float *)malloc(sizeof(float)*num_channels*TT);
                mda_read_float32(buf,&HH,num_channels*TT,inf);
                for (int jj=0; jj<num_jobs; jj++) {
                    JOB *X=&jobs[jj];
                    FILE *outf=fopen(X->input_path.toLatin1().data(),"a");
                    float *buf2=(float *)malloc(sizeof(float)*X->channels.count()*TT);
                    for (int tt=0; tt<TT; tt++) {
                        for (int aa=0; aa<X->channels.count(); aa++) {
                            buf2[aa+tt*X->channels.count()]=buf[X->channels[aa]+tt*num_channels];
                        }
                    }
                    printf("WRITING %d bytes\n",X->channels.count()*TT*HH.num_bytes_per_entry);
                    mda_write_float32(buf2,&HH,X->channels.count()*TT,outf);
                    free(buf2);
                    fclose(outf);
                    launch_worker(X,TT);
                    X->file_pos+=X->channels.count()*TT*sizeof(float);
                }
                free(buf);
                timepoint+=TT;
            }
            else {
                //sleep
            }
        }


        fclose(inf);
        return 0;
    }
}
