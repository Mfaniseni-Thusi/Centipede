#ifndef LOGIC_H
#define LOGIC_H

#include "Grid.h"
#include "Presentation.h"
#include "HighScoreManager.h"
#include "ScreenState.h"
#include "Player.h"
#include "StopWatch.h"
#include "EnemyFactory.h"
#include "CollisionHandler.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <ctime>

using std::vector;
using std::shared_ptr;

/** \class Logic
 *  \brief This class controls the behaviour of the game objects created
 *  before they get displayed on the screen.
 *  \author 1043475 Lynch Mwaniki and 1076467 Madimetja Sethosa.
 *  \version 3.0
 */

class Logic
{
    public:
        /** \brief Default Logic constructor. An instance of the game is created
         *  and the game objects are initialized.
         */
        Logic();

        /** \brief Runs the game loop.
         */
        void run();

        /** \brief Default Destructor. Destroys an instance of the game.
         */
        ~Logic();

    private:
        const Grid grid_{592, 640};
        Presentation presentation_{grid_.getWidth(), grid_.getHeight()};
        HighScoreManager highScoreManager_;
        AssetManager assetManager_;
        EnemyFactory enemyFactory_{grid_};
        CollisionHandler collisionHandler_{grid_};
        shared_ptr<Player> player_;
        vector<shared_ptr<IEntity>> game_objects_;
        vector<shared_ptr<IMovingEntity>> moving_game_objects_;
        int high_score_;
        ScreenState screen_state_;
        bool debounceSpaceKey_;

        // Private functions:

        /** \brief Displays the splash screen at the start of the game.
         */
        void renderSplashScreen();

        /** \brief Tells presentation to process input from keyboard.
         */
        void getInputCommands();

        /** \brief Loads paths to the textures used by the game.
         */
        void loadAssets();

        /** \brief Gives Presentation a vector of game objects to be drawn.
         */
        void renderGameObjects();

        /** \brief Displays the game over screen when the player has lost the game.
         */
        void renderGameOverScreen();

        /** \brief Displays the game won screen when the player has won the game.
         */
        void renderGameWonScreen();

        /** \brief Moves all the game objects that can be moved.
         */
        void updateGameObjects();

        /** \brief generates the a centipede with one head, and multiple body segments.
         */
        void generateNormalCentipede();

        /** \brief generates centipede heads.
         */
        void generateCentipedeHeads();

        /** \brief generates mushrooms
         */
        void generateMushrooms();

        /** \brief Checks for collisions between game objects.
         */
        void checkCollisions();

        /** \brief Checks whether the player's score has passed the current high
         * score and updates the high score if true.
         */
        void updateScores();

        /** \brief Removes all dead entities from vector(s) where they exist.
         */
        void removeDeadEntities();

        /** \brief Removes all centipede segments from vector(s) where they exist,
         *      reset the position of the player and generate a new centipede train.
         */
        void reincarnatePlayer();

        /** \brief transforms all dead centipede segments from vector(s) into mushrooms.
         */
        void generateMushroomAtCollision();

        /** \brief generates a scorpion at a random position.
         */
        void generateAScorpion();


};

/**\brief  container_erase_if - a function that erases elements in a container based on a
 * specified condition determined by the Predicate.
 *
 * \param container a std container passed by reference to be iterated through.
 * \param predicate can be a function that returns true or false.
 */
template<typename TypeContainer, typename PredicateT>
inline void container_erase_if(TypeContainer& container, const PredicateT& predicate)
{
    container.erase(remove_if(begin(container),
                              end(container), predicate),
                    end(container));
}//

#endif // LOGIC_H
