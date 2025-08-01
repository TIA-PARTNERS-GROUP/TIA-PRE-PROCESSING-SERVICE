#include <optional>
#include <string>

// --- Primary Tables ---

struct User {
  int id;
  std::optional<std::string> first_name;
  std::optional<std::string> last_name;
  std::optional<std::string> contact_email;
  std::optional<std::string> contact_phone_no;
};

struct BusinessCategory {
  int id;
  std::optional<std::string> name;
};

struct BusinessPhase {
  int id;
  std::optional<std::string> name;
};

struct BusinessRole {
  int id;
  std::optional<std::string> name;
};

struct BusinessSkill {
  int id; // Note: autoincrement was false
  std::optional<std::string> name;
};

struct BusinessType {
  int id;
  std::optional<std::string> name;
};

struct ConnectionType {
  int id;
  std::optional<std::string> name;
};

struct DailyActivity {
  int id;
  std::optional<std::string> name;
  std::optional<std::string> description;
};

struct IndustryCategory {
  int id;
  std::optional<std::string> name;
};

struct MastermindRole {
  int id;
  std::optional<std::string> name;
};

struct Region {
  std::string id; // CHAR(3)
  std::optional<std::string> name;
};

struct StrengthCategory {
  int id;
  std::optional<std::string> name;
};

struct Subscription {
  int id;
  std::optional<std::string> name;
  std::optional<double> price; // TODO  dedicated library for financial
                               // precision
  std::optional<int> valid_days;
  std::optional<int> valid_months;
};

// --- Tables with Foreign Keys ---

struct Business {
  int id;

  std::optional<int> operator_user_id;
  std::optional<std::string> name;
  std::optional<std::string> tagline;
  std::optional<std::string> website;
  std::optional<std::string> description;
  std::optional<std::string> address;
  std::optional<std::string> city;
  std::optional<int> business_type_id;
  std::optional<int> business_category_id;
  std::optional<int> business_phase;
};

struct BusinessStrength {
  int id;
  std::optional<int> business_role_id;
  std::optional<int> business_phase_id;
  std::optional<std::string> name;
};

struct CaseStudy {
  int id;
  std::optional<int> owner_user_id;
  std::optional<std::string> title;
  std::optional<std::string> content;
  std::optional<std::string> thumbnail;
  std::optional<std::string> video_url;
  std::optional<bool> published;
};

struct Idea {
  int id; // Note: autoincrement was false
  std::optional<int> submitted_by_user_id;
  std::optional<std::string> content;
};

struct Industry {
  int id;
  std::optional<int> category_id;
  std::optional<std::string> name;
  std::optional<std::string> picture;
};

struct SkillCategory {
  int id;
  std::optional<std::string> name;
  std::optional<int> business_type_id;
};

struct Strength {
  int id;
  std::optional<int> category_id;
  std::optional<std::string> name;
};

struct UserLogin {
  int user_id;
  std::optional<std::string> login_email;
  std::optional<std::string>
      password_hash; // Representing Binary as string for simplicity
  std::optional<std::string> password_reset_token;
  std::optional<std::string> password_reset_requested_timestamp;
};

struct Notification {
  int id;
  std::optional<int> sender_user_id;
  std::optional<int> receiver_user_id;
  std::optional<std::string> message;
  std::optional<std::string> time_sent;
  std::optional<bool> opened;
};

struct Project {
  int id;
  std::optional<int> managed_by_user_id;
  std::optional<std::string> name;
  std::optional<std::string> project_status; // ENUM as string
  std::optional<std::string> projection_open;
  std::optional<std::string> project_closed;
  std::optional<std::string> project_completion;
};

struct Skill {
  int id;
  std::optional<int> category_id;
  std::optional<std::string> name;
  std::optional<std::string> picture;
};

struct UserPost {
  int id;
  std::optional<int> poster_user_id;
  std::optional<std::string> title;
  std::optional<std::string> content;
  std::optional<std::string> thumbnail;
  std::optional<std::string> video_url;
  std::optional<bool> published;
};

// --- Many-to-Many / Join Tables ---

struct BusinessConnection {
  int id;
  std::optional<int> initiating_business_id;
  std::optional<int> receiving_business_id;
  std::optional<int> connection_type_id;
  std::optional<bool> active;
  std::optional<std::string> date_initiated;
};

struct DailyActivityEnrolment {
  int daily_activity_id;
  int user_id;
};

struct IdeaVote {
  int voter_user_id;
  int idea_id;
  std::optional<bool> type;
};

struct UserBusinessStrength {
  int business_strength_id;
  int user_id;
};

struct UserDailyActivityProgress {
  std::string date;
  int user_id;
  int daily_activity_id;
  std::optional<int> progress;
};

struct UserStrength {
  int strength_id;
  int user_id;
};

struct UserSubscription {
  int user_id;
  int subscription_id;
  std::optional<std::string> date_from;
  std::optional<std::string> date_to;
  std::optional<double> price;
  std::optional<double> total;
  std::optional<double> tax_amount;
  std::optional<double> tax_rate;
  std::optional<std::string> trial_from;
  std::optional<std::string> trial_to;
};

struct ConnectionMastermindRole {
  int connection_id;
  int mastermind_role_id;
};

struct ProjectBusinessCategory {
  int project_id;
  int business_category_id;
};

struct ProjectBusinessSkill {
  int project_id;
  int business_skill_id;
};

struct ProjectRegion {
  std::string region_id;
  int project_id;
};

struct UserSkill {
  int skill_id;
  int user_id;
};
