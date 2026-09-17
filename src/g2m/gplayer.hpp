/*
 *  Modification & Copyright 2015      Kazuyasu Hamada (k-hamada@gifu-u.ac.jp)
*/

/// \file
/// Steps through a finished tool path, for animation.

#ifndef GPLAYER_HH
#define GPLAYER_HH

#include <vector>
#include <limits.h>
#include <iostream>
#include <fstream>

#include <QString>
#include <QObject>
#include <QtDebug>

#include "canonLine.hpp"
#include "nanotimer.hpp"

/// distance between samples along a move, in machine units
#define DEFAULT_STEP_SIZE (0.1)
/// feed rate assumed when the program does not set one
#define DEFAULT_FEED_RATE       (200.0)
/// rate rapids are played back at
#define DEFAULT_TRAVERSE_FEED_RATE      (1000.0)
/// how close two positions have to be before they count as the same
#define TOLERANCE       (1e-2)
/// milliseconds between animation steps
#define DEFAULT_ANIMATE_INTERVAL        (3)

namespace g2m {

/// whether a move goes into the work, and which way
enum PLUNGE_STATUS {
    NO_PLUNGE = 0x0,            ///< no motion along the tool axis
    POSITIVE_PLUNGE = 0x10000,  ///< retracting
    NEGATIVE_PLUNGE = 0x20000,  ///< plunging into the work
};

/**
\class GPlayer
\brief This class loops through the list of canonLine objects produced by g2m.
*/
class GPlayer : public QObject {
    Q_OBJECT;

    public:
        /// Sets the step size, feed rates and interval to their defaults.
        GPlayer()  {  
            first = true;
            current_line = 0;
            inv_ds = 1.0 / DEFAULT_STEP_SIZE;
            m = 0;
            play_flag = false;
            feed_rate = DEFAULT_FEED_RATE;
            traverse_feed_rate = DEFAULT_TRAVERSE_FEED_RATE;
            total_length = 0.0;
            total_time = 0.0;
        }

        /// \param ds distance between samples; ignored when not positive
        void setStepSize(double ds) {
        	if (ds > 0.0)
        		inv_ds = 1.0 / ds;
        }

        /// \param rate rate to play rapids back at; ignored when not positive
        void setTraverseFeedRate(double rate) {
        	if (rate > 0.0)
        		traverse_feed_rate = rate;
        }

        /// \param line index into the program
        /// \returns that canon line; the index is not checked
        canonLine* getCanonLine(unsigned int line) { return lines[line]; }

    public slots:
        /// start or resume executing the program
        void play() {
        	if (play_flag == false) {
        		play_flag = true;
        		qDebug() << " gplayer::play() ";
        		emit debugMessage( tr("GPlayer: play") );
        		slotRequestMove();
        	}
        }
        /// signal the next move
        void slotRequestMove() {
        	if (!(lines.size() > 0)) { play_flag = false; return; }
        	if (current_line >= (lines.size()-1)) { play_flag = false; return; }
        	if (play_flag == false) {
        		if (lines.size() - 1)
        			emit signalProgress( (int)(100*current_line/(lines.size()-1)) , current_line, total_time, true); // report progress to ui

        		return;
        	}
            // UI request that we signal the next signalToolPosition()
            canonLine* cl = lines[current_line];

            if (first) {// first ever call here
                current_tool = cl->getStatus()->getTool();
                first = false;
            }   
            if (cl->isMotion() ) {
                // divide motion into sampled points. signal motion along path.
            	if (m == 0) {
            		move_length = cl->length();
            		n_samples = std::max( (int)( move_length * inv_ds ) , 2 ); // want at least two points: start-end
            		interval_size = move_length /(double)(n_samples-1);
            		motionStatus = cl->getStatus()->getSpindleMotionStatus();
            		feed_rate = cl->getStatus()->getFeed();
            		if (feed_rate <= 0.0) feed_rate = DEFAULT_FEED_RATE;
            		double diff_z = cl->point((n_samples-1)*interval_size).z - cl->point(0.0).z;
            		plunge = (diff_z > TOLERANCE) ? POSITIVE_PLUNGE : (diff_z < -TOLERANCE) ? NEGATIVE_PLUNGE : NO_PLUNGE;
            		motionStatus |= plunge;
            	}
                // FIXME: handle first and last moves differently?
            	Point pos = cl->point( (double)(m) * interval_size );
#ifdef MULTI_AXIS
            	Point angle = cl->angle( (double)(m) * interval_size );
#endif
                if (m == (n_samples-1) )
                    move_done = true;
#ifdef MULTI_AXIS
                emit signalToolPosition( pos.x, pos.y, pos.z, angle.x, angle.y, angle.z, current_line, motionStatus, feed_rate );
#else
                emit signalToolPosition( pos.x, pos.y, pos.z, current_line, motionStatus, feed_rate);
#endif
                m++; // advance along the move
            } else {
                // not motion, check for toolchange
                if ( current_tool != cl->getStatus()->getTool() ) {
                   emit signalToolChange( cl->getStatus()->getTool() );
                   current_tool = cl->getStatus()->getTool();
                }
                current_line++;
                slotRequestMove(); // call myself!
           }
            
            if (move_done) {
				if (cl->isMotion()) {
					total_length += move_length;
					if (cl->getMotionType() == TRAVERSE)
						total_time += move_length / traverse_feed_rate;
					else
						total_time += move_length / feed_rate;
				}
                current_line++;
                move_done = false;
                m = 0;
            }
            if (lines.size() - 1)
            	if (m == 0 || (m & DEFAULT_ANIMATE_INTERVAL) == 0x0) {
            		emit signalProgress( (int)(100*current_line/(lines.size()-1)) , current_line, total_time, current_line == (lines.size()-1)); // report progress to ui
            	}
        }
        /// pause program
        void pause() {
        	play_flag = false;
            emit debugMessage( tr("GPlayer: pause") );
        }
        /// stop the execution of the g-code program
        void stop() {
        	if (play_flag == false)
        		emit signalProgress(0, 0, 0.0, true);
        	play_flag = false;
        	first = true;
        	current_line = 0;
        	m = 0;
        	total_time = 0;
            emit debugMessage( tr("GPlayer: stop") );
        }
        /// add a canonLine to the vector of lines to be processed
        /// \param l the new canonLine to be added
        void appendCanonLine( canonLine* l) {
            lines.push_back(l);
        }

    signals:
        /// signal a new tool position
#ifdef MULTI_AXIS
    	void signalToolPosition( double x, double y, double z, double a, double b, double c, int line, int mstatus, double feedrate ); // 5-axis for now..
#else
        void signalToolPosition( double x, double y, double z, int line, int mstatus, double feedrate ); // 3-axis.
#endif
        /// signal a tool change to new tool \param t
        void signalToolChange( int t );
        /// signal the UI how far along the g-code program we are
        void signalProgress( int p, int line, double time, bool force );
        /// signal a debug message
        void debugMessage(QString s);

    protected:
        /// flag for first move of g-code
        bool first;
        /// index of current tool
        int current_tool;
        /// the current canonLine being processed
        unsigned int current_line;
        /// loop variable
        int    m;
        /// length of the move being played
        double move_length;
        /// how many samples that move is split into
        int    n_samples;
        /// distance between those samples
        double interval_size;
        /// flag indicating when current move done
        bool move_done;
        /// vector of canonLines to process
        std::vector<canonLine*> lines;

        /// true while the program is playing
        bool play_flag;

    private:
        int    plunge;             ///< PLUNGE_STATUS of the current move
        int    motionStatus;       ///< spindle status and motion type, or-ed
        double inv_ds;             ///< reciprocal of the sample step size
		double feed_rate;          ///< feed rate moves are played back at
		double traverse_feed_rate; ///< rate rapids are played back at
        double total_length;       ///< length of the whole program
		double total_time;         ///< how long playing it takes
};

} // end namespace
#endif // GPlayer_HH
