#include "CollisionHandler.h"
#include <cmath>
CollisionHandler::CollisionHandler(const Grid& grid):
spatial_hash_{grid}, points_obtained_{0}
{

}

int CollisionHandler::getPointsObtained()
{
    auto points = points_obtained_;
    points_obtained_ = 0;
    return points;
}

int CollisionHandler::countObjects(vector<IMovingEntity_ptr>::iterator game_objects_begin,
                                   vector<IMovingEntity_ptr>::iterator game_objects_end,
                                   ObjectType object_type)
{
    return (count_if(game_objects_begin,
                     game_objects_end,
                     [&, object_type](const IMovingEntity_ptr& object)
                     {
                        return(object->getObjectType()==object_type);
                     }));
}

vector<IMovingEntity_ptr> CollisionHandler::copyObjects(vector<IMovingEntity_ptr>::iterator game_objects_begin,
                                                        vector<IMovingEntity_ptr>::iterator game_objects_end,
                                                        ObjectType object_type)
{
    auto number_of_objects = countObjects(game_objects_begin, game_objects_end,
                                          object_type);

    vector<IMovingEntity_ptr> copied_elements(number_of_objects);
    copy_if(game_objects_begin,
            game_objects_end,
            copied_elements.begin(),
            [&, object_type](const IMovingEntity_ptr& object)
            {
                return(object->getObjectType()==object_type);
            });
    return copied_elements;
}

void CollisionHandler::checkCollisions(vector<IEntity_ptr>& game_objects,
                                       vector<IMovingEntity_ptr>& moving_game_objects)
{
    // Generate Spatial Hash:
    spatial_hash_.generateSpatialHashTable(game_objects);

    auto player_bullets = copyObjects(moving_game_objects.begin(), moving_game_objects.end(),
                ObjectType::PLAYER_LASER_BULLET);

    auto centipede = copyObjects(moving_game_objects.begin(), moving_game_objects.end(),
                ObjectType::CENTIPEDE);
    auto player = copyObjects(moving_game_objects.begin(), moving_game_objects.end(),
                ObjectType::PLAYER);
    auto scorpion = copyObjects(moving_game_objects.begin(), moving_game_objects.end(),
                ObjectType::SCORPION);

    centipedeCollidesWithCentipede(centipede);
    playerCollidesWithObjects(player);
    playerBulletCollidesWithEnemies(player_bullets);
    playerBulletCollidesWithCentipede(player_bullets, centipede);
    centipedeCollidesWithMushroom(centipede);
    scorpionCollidesWithMushroom(scorpion);
}

void CollisionHandler::playerCollidesWithObjects(vector<IMovingEntity_ptr>& player)
{
    for(auto& thePlayer : player)
    {
        auto near_by_objects = spatial_hash_.retrieveNearbyObjects(thePlayer);
        for(auto& object : near_by_objects)
        {
            if(!thePlayer->isAlive()) return;
            if(sat_algorithm_.checkOverlap(thePlayer->getBoundaryBox(), object->getBoundaryBox()))
            {
                if(object->getObjectType() == ObjectType::MUSHROOM)
                {
                    // Resolve collision if penetration is greater than zero in x or y:
                    Direction direction;
                    if(thePlayer->getDirection()==Direction::UP)
                        direction = Direction::DOWN;
                    else if (thePlayer->getDirection()==Direction::DOWN)
                        direction = Direction::UP;
                    else if(thePlayer->getDirection()==Direction::LEFT)
                        direction = Direction::RIGHT;
                    else if (thePlayer->getDirection()==Direction::RIGHT)
                        direction = Direction::LEFT;

                    // Move Player by at least one point
                    thePlayer->setDirection(direction);
                    thePlayer->move();
                    thePlayer->move();
                    thePlayer->setDirection(Direction::NONE);
                }
                else if(object->getObjectType()!=ObjectType::PLAYER_LASER_BULLET)
                { // Anything else will kill player immediately besides its bullets

                    object->eliminated();
                    thePlayer->eliminated();
                    return;
                }//else
            }//if
        }//for
    }//for
}

void CollisionHandler::playerBulletCollidesWithEnemies(vector<IMovingEntity_ptr>& player_bullets)
{
    for(auto& bullet : player_bullets){
        auto near_by_objects = spatial_hash_.retrieveNearbyObjects(bullet);
        for(auto& object : near_by_objects)
        {
            if(!bullet->isAlive()) break;
            if(object->getObjectType() != ObjectType::CENTIPEDE &&
               object->getObjectType() != ObjectType::PLAYER &&
               object->getObjectType() != ObjectType::PLAYER_LASER_BULLET &&
               object->isAlive())
            {
                if(sat_algorithm_.checkOverlap(bullet->getBoundaryBox(), object->getBoundaryBox()))
                {
                    bullet->eliminated();
                    object->eliminated();
                    // Get points:
                    switch(object->getObjectType())
                    {
                        case ObjectType::MUSHROOM :
                            if(object->isPoisoned()) points_obtained_+=2;
                            else points_obtained_+=1;
                            break;

                        case ObjectType::SCORPION:
                            points_obtained_+=40;
                            break;

                        default:
                            break;
                    }//switch
                }//if
            }//if
        }//for
    }//for
}

void CollisionHandler::playerBulletCollidesWithCentipede(vector<IMovingEntity_ptr>& player_bullets,
                                                         vector<IMovingEntity_ptr>& centipede)
{
    for(auto& bullet : player_bullets){
        auto near_by_objects = spatial_hash_.retrieveNearbyObjects(bullet);
        for(auto& object : near_by_objects)
        {
            if(!bullet->isAlive()) break;
            if(object->getObjectType() == ObjectType::CENTIPEDE && object->isAlive())
            {
                if(sat_algorithm_.checkOverlap(bullet->getBoundaryBox(), object->getBoundaryBox()))
                {
                    bullet->eliminated();
                    object->eliminated();
                    points_obtained_+=100;

                    //Reorder centipede:
                    auto iter_new_head = find(centipede.begin(), centipede.end(), object);
                    auto iter_segment  = find(centipede.begin(), centipede.end(), object);
                    ++iter_new_head;

                    if(iter_new_head!=centipede.end())
                    {
                        auto centipede_seg_ptr = dynamic_pointer_cast<CentipedeSegment>(*iter_new_head);
                        centipede_seg_ptr->setBodyType(CentipedeSegment::BodyType::HEAD);
                        auto centipede_new_head_y_pos = centipede_seg_ptr->getPosition().getY_pos();

                        // Go to next segment:
                        ++iter_segment;
                        // Update train:
                        for(++iter_segment; iter_segment!=centipede.end(); ++iter_segment)
                        {
                            auto centipede_seg_ptr = dynamic_pointer_cast<CentipedeSegment>(*iter_segment);
                            if(centipede_seg_ptr->getBodyType() == CentipedeSegment::BodyType::HEAD) break;

                            if(centipede_seg_ptr->isAlive()
                               && centipede_seg_ptr->getPosition().getY_pos() == centipede_new_head_y_pos)
                                centipede_seg_ptr->clearHeadCollisions();
                        }//for
                    }//if
                }//if
            }//if
        }//for
    }//for
}

void CollisionHandler::centipedeCollidesWithCentipede(vector<IMovingEntity_ptr>& centipede)
{
    vector<IMovingEntity_ptr> centipede_heads_collided;
    for(auto& segment : centipede)
    {
        auto found = std::count(centipede_heads_collided.begin(), centipede_heads_collided.end(), segment);
        auto centipede_head_ptr = dynamic_pointer_cast<CentipedeSegment>(segment);
        if(segment->isAlive() && found==0
           && centipede_head_ptr->getBodyType()==CentipedeSegment::BodyType::HEAD)
        {
            auto near_by_objects = spatial_hash_.retrieveNearbyObjects(segment);
            for(auto& object : near_by_objects)
            {
                if(object->isAlive() && object->getObjectType()==ObjectType::CENTIPEDE)
                {
                    if(sat_algorithm_.checkOverlap(segment->getBoundaryBox(), object->getBoundaryBox()))
                    {
                        centipede_heads_collided.push_back(segment);

                        auto iter_segment = find(centipede.begin(), centipede.end(), segment);
                        for(++iter_segment; iter_segment!=centipede.end(); ++iter_segment)
                        {
                            auto centipede_seg_ptr = dynamic_pointer_cast<CentipedeSegment>(*iter_segment);
                            if(centipede_seg_ptr->getBodyType()==CentipedeSegment::BodyType::HEAD) break;

                            if(centipede_seg_ptr->isAlive())
                            {
                                centipede_seg_ptr->collisionAt(centipede_head_ptr->getPosition(), false);
                                centipede_seg_ptr->move();
                            }
                            centipede_head_ptr->changeDirection();
                            centipede_head_ptr->move();
                        }//for
                    }//if
                }//if
            }//for
        }//if
    }//for
}

void CollisionHandler::centipedeCollidesWithMushroom(vector<IMovingEntity_ptr>& centipede)
{
    for(auto& segment : centipede)
    {
        auto centipede_ptr = dynamic_pointer_cast<CentipedeSegment>(segment);
        if(segment->isAlive() &&
           centipede_ptr->getBodyType()==CentipedeSegment::BodyType::HEAD &&
           !segment->isPoisoned())
        {
            auto near_by_objects = spatial_hash_.retrieveNearbyObjects(segment);
            for(auto& object : near_by_objects)
            {
                if(object->isAlive() && object->getObjectType()==ObjectType::MUSHROOM)
                {
                    if(sat_algorithm_.checkOverlap(segment->getBoundaryBox(), object->getBoundaryBox()))
                    {
                        auto iter_segment = find(centipede.begin(), centipede.end(), segment);
                        // Update train of bodies:
                        for(++iter_segment; iter_segment!=centipede.end(); ++iter_segment)
                        {
                            auto centipede_seg_ptr = dynamic_pointer_cast<CentipedeSegment>(*iter_segment);
                            if(centipede_seg_ptr->getBodyType()==CentipedeSegment::BodyType::HEAD) break;

                            if(centipede_seg_ptr->isAlive())
                            {
                                centipede_seg_ptr->collisionAt(centipede_ptr->getPosition(),
                                                               object->isPoisoned());
                                centipede_seg_ptr->move();
                                centipede_seg_ptr->move();
                            }//if
                        }//for

                        if(object->isPoisoned())
                            segment->poison();
                        else
                        {
                            centipede_ptr->changeDirection();
                            centipede_ptr->move();
                            centipede_ptr->move();
                        }
                    }//if
                }//if
            }//for
        }//if
    }//for
}

void CollisionHandler::scorpionCollidesWithMushroom(vector<IMovingEntity_ptr>& scorpions)
{
   for(auto& scorpion :scorpions)
   {
       if(scorpion->isAlive())
       {
          auto near_by_objects = spatial_hash_.retrieveNearbyObjects(scorpion);
          for(auto& object: near_by_objects)
          {
              if(object->isAlive() && object->getObjectType()==ObjectType::MUSHROOM)
              {
                 if(sat_algorithm_.checkOverlap(scorpion->getBoundaryBox(), object->getBoundaryBox()))
                    object->poison();
              } //if
          } // for
       }//if
   }//for
}

CollisionHandler::~CollisionHandler()
{
    //dtor
}
