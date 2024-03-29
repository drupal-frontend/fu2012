<?php

/**
 * Form builder; Twitter settings form.
 */
function twitter_admin_form($form, &$form_state) {
  $form['twitter_import'] = array(
    '#type' => 'checkbox',
    '#title' => t('Import and display the Twitter statuses of site users who have entered their Twitter account information.'),
    '#default_value' => variable_get('twitter_import', 1),
  );
  $form['twitter_expire'] = array(
    '#type' => 'select',
    '#title' => t('Delete old statuses'),
    '#default_value' => variable_get('twitter_expire', 0),
    '#options' => array(0 => t('Never')) + drupal_map_assoc(array(604800, 2592000, 7776000, 31536000), 'format_interval'),
    '#states' => array(
      'visible' => array(
        ':input[name=twitter_import]' => array('checked' => TRUE),
      ),
    ),
  );

  $form['oauth'] = array(
    '#type' => 'fieldset',
    '#title' => t('OAuth Settings'),
    '#access' => module_exists('oauth_common'),
    '#description' => t('To enable OAuth based access for twitter, you must <a href="@url">register your application</a> with Twitter and add the provided keys here.', array('@url' => 'https://dev.twitter.com/apps/new')),
  );
  $form['oauth']['callback_url'] = array(
    '#type' => 'item',
    '#title' => t('Callback URL'),
    '#markup' => url('twitter/oauth', array('absolute' => TRUE)),
  );
  $form['oauth']['twitter_consumer_key'] = array(
    '#type' => 'textfield',
    '#title' => t('OAuth Consumer key'),
    '#default_value' => variable_get('twitter_consumer_key', NULL),
  );
  $form['oauth']['twitter_consumer_secret'] = array(
    '#type' => 'textfield',
    '#title' => t('OAuth Consumer secret'),
    '#default_value' => variable_get('twitter_consumer_secret', NULL),
  );

  return system_settings_form($form);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function twitter_user_settings($account) {
  module_load_include('inc', 'twitter');

  $output = array();
  if (!empty($account->twitter_accounts)) {
    $output['list_form'] = drupal_get_form('twitter_account_list_form', $account->twitter_accounts);
  }
  $output['form'] = drupal_get_form('twitter_account_form', $account);

  return $output;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function twitter_account_list_form($form, $form_state, $twitter_accounts = array()) {
  $form['#tree'] = TRUE;
  $form['accounts'] = array();

  foreach ($twitter_accounts as $twitter_account) {
    $form['accounts'][] = _twitter_account_list_row($twitter_account);
  }

  if (!empty($twitter_accounts)) {
    $form['buttons']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save changes'),
    );
  }

  return $form;
}

function _twitter_account_list_row($account) {
  $form['#account'] = $account;

  $form['id'] = array(
    '#type' => 'value',
    '#value' => $account->id,
  );

  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $account->uid,
  );

  $form['screen_name'] = array(
    '#type' => 'value',
    '#value' => $account->screen_name,
  );

  $form['image'] = array(
    '#markup' => theme('image', array('path' => $account->profile_image_url)),
  );

  $form['visible_name'] = array(
    '#markup' => l($account->screen_name, 'http://www.twitter.com/' . $account->screen_name),
  );

  $form['description'] = array(
    '#markup' => filter_xss($account->description),
  );

  $form['protected'] = array(
    '#markup' => empty($account->protected) ? t('No') : t('Yes'),
  );

  // Here we use user_access('import own tweets') to check permission instead
  // of user_access('import own tweets', $account) is because we allow roles
  // with sufficient permission to overwrite the user's import settings.
  if (user_access('import own tweets')) {
    $form['import'] = array(
      '#type' => 'checkbox',
      '#default_value' => user_access('import own tweets') ? $account->import : '',
    );
  }

  $form['delete'] = array(
    '#type' => 'checkbox',
  );

  return $form;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function theme_twitter_account_list_form($variables) {
  $form = $variables['form'];

  if (user_access('import own tweets')) {
    $header = array('', t('Name'), t('Description'), t('Private'), t('Import'), t('Delete'));
  }
  else {
    $header = array('', t('Name'), t('Description'), t('Private'), t('Delete'));
  }

  if (user_access('make twitter accounts global')) {
    $header[] = '';
  }

  $rows = array();

  foreach (element_children($form['accounts']) as $key) {
    $element = &$form['accounts'][$key];
    if (user_access('import own tweets')) {
      $row = array(
        drupal_render($element['image']),
        drupal_render($element['id']) . drupal_render($element['screen_name']) . drupal_render($element['visible_name']),
        drupal_render($element['description']),
        drupal_render($element['protected']),
        drupal_render($element['import']),
        drupal_render($element['delete']),
      );
    }
    else {
      $row = array(
        drupal_render($element['image']),
        drupal_render($element['id']) . drupal_render($element['screen_name']) . drupal_render($element['visible_name']),
        drupal_render($element['description']),
        drupal_render($element['protected']),
        drupal_render($element['delete']),
      );
    }

    if (user_access('make twitter accounts global')) {
      $label = ($element['#account']->is_global) ? t('remove global') : t('make global');
      $row[] = l($label, 'user/' . $element['#account']->uid . '/edit/twitter/global/' . $element['#account']->id);
    }

    $rows[] = $row;
  }

  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= drupal_render_children($form);
  return $output;
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function twitter_account_list_form_submit($form, &$form_state) {
  $accounts = $form_state['values']['accounts'];
  foreach ($accounts as $account) {
    if (empty($account['delete'])) {
      twitter_account_save($account);
    }
    else {
      twitter_account_delete($account['id'], $account['screen_name']);
    }
  }
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function twitter_user_make_global($form, $form_state, $account, $twitter_uid) {
  module_load_include('inc', 'twitter');

  $twitter_account = twitter_account_load($twitter_uid);

  $form = array();

  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $account->uid,
  );

  $form['twitter_uid'] = array(
    '#type' => 'value',
    '#value' => $twitter_uid,
  );

  if ($twitter_account->is_global) {
    $text = t('Are you sure you want to remove %screen_name from the global accounts?', array('%screen_name' => $twitter_account->screen_name));
    $description = t('This means other users will no longer be allowed to post using this account.');
  }
  else {
    $text = t('Are you sure you want to allow other users to access the %screen_name account?', array('%screen_name' => $twitter_account->screen_name));
    $description = t('This will allow other users to post using this account.');
  }

  return confirm_form($form, $text, 'user/' . $account->uid . '/edit/twitter', $description);
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function twitter_user_make_global_submit($form, &$form_state) {
  db_update('twitter_account')
    ->expression('is_global', '(1 - is_global)')
    ->condition('twitter_uid', $form_state['values']['twitter_uid'])
    ->execute();

  $form_state['redirect'] = 'user/' . $form_state['values']['uid'] . '/edit/twitter';
}

/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function twitter_account_form($form, $form_state, $account = NULL) {
  if (empty($account)) {
    global $user;
    $account = $user;
  }

  $form['uid'] = array(
    '#type' => 'value',
    '#value' => $account->uid,
  );

  if (_twitter_use_oauth()) {
    $form['#validate'] = array('twitter_account_oauth_validate');
  }
  else {
    $form['screen_name'] = array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#title' => t('Twitter user name'),
    );

    $form['import'] = array(
      '#type' => 'checkbox',
      '#title' => t('Import statuses from this account'),
      '#default_value' => TRUE,
      '#access' => FALSE,
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add account'),
  );

  return $form;
}

/**
 * Implements hook_FORM_ID_submit()
 *
 * Loads Twitter account details and adds them to the user account
 */
function twitter_account_form_submit($form, &$form_state) {
  module_load_include('lib.php', 'twitter');
  module_load_include('inc', 'twitter');

  $name = $form_state['values']['screen_name'];
  $twitter = new Twitter($name);
  $account = $twitter->users_show($name, false);
  $account->set_auth($form_state['values']);
  twitter_account_save($account, TRUE);
}

/**
 * If OAuth is enabled, intercept submission of 'Add Account' form on
 * user/%/edit/twitter page and redirect to Twitter for auth.
 */
function twitter_account_oauth_validate($form, &$form_state) {
  module_load_include('lib.php', 'oauth_common');
  module_load_include('lib.php', 'twitter');

  $key = variable_get('twitter_consumer_key', '');
  $secret = variable_get('twitter_consumer_secret', '');
  if ($key == '' || $secret == '') {
    form_set_error('', t('Please configure your Twitter consumer key and secret.'));
  }

  $twitter = new TwitterOAuth($key, $secret);
  $token = $twitter->get_request_token();

  $_SESSION['twitter_oauth']['account'] = user_load($form['uid']['#value']);  
  $_SESSION['twitter_oauth']['token'] = $token;
  $_SESSION['twitter_oauth']['destination'] = $_GET['q'];
  drupal_goto($twitter->get_authorize_url($token));
}

/**
 * @TODO This code should probably be reviewed.
 *
 * Wrapper to call drupal_form_submit() which wasn't required in D6.
 */
function twitter_oauth_callback() {
  $form_state['values']['oauth_token'] = $_GET['oauth_token'];
  drupal_form_submit('twitter_oauth_callback_form', $form_state);
}

/**
 * Form builder function. In D6 this form was built in response to the
 * oauth return request from Twitter, and the setting of
 * $form['#post'] seems to have caused the form to be validated and
 * processed.
 */
function twitter_oauth_callback_form($form, &$form_state) {
  $form['#post']['oauth_token'] = $_GET['oauth_token'];
  $form['oauth_token'] = array(
    '#type' => 'hidden',
    '#default_value' => $_GET['oauth_token'],
  );
  return $form;
}

/**
 * Validate results from Twitter OAuth return request.
 */
function twitter_oauth_callback_form_validate($form, &$form_state) {
  $key = variable_get('twitter_consumer_key', '');
  $secret = variable_get('twitter_consumer_secret', '');

  if ($key == '' || $secret == '') {
    form_set_error('', t('Please configure your Twitter consumer key and secret.'));
  }

  if (isset($_SESSION['twitter_oauth'])) {
    $form_state['twitter_oauth'] = $_SESSION['twitter_oauth'];
    unset($_SESSION['twitter_oauth']);
  }
  else {
    form_set_error('oauth_token', 'Invalid Twitter OAuth request');
  }

  if (isset($form_state['twitter_oauth']['token'])) {
    $token = $form_state['twitter_oauth']['token'];
    if (!is_array($token) || !$key || !$secret) {
      form_set_error('oauth_token', t('Invalid Twitter OAuth request'));
    }
    if ($token['oauth_token'] != $form_state['values']['oauth_token']) {
      form_set_error('oauth_token', t('Invalid OAuth token.'));
    }
  }
  else {
    form_set_error('oauth_token', t('Invalid Twitter OAuth request'));
  }

  module_load_include('lib.php', 'oauth_common');
  module_load_include('lib.php', 'twitter');
  module_load_include('inc', 'twitter');

  if ($twitter = new TwitterOAuth($key, $secret, $token['oauth_token'], $token['oauth_token_secret'])) {
    if ($response = $twitter->get_access_token()) {
      $form_state['twitter_oauth']['response'] = $response;
    }
    else {
      form_set_error('oauth_token', t('Invalid Twitter OAuth request'));
    }
  }
  else {
    form_set_error('oauth_token', t('Invalid Twitter OAuth request'));
  }
}

/**
 * Handle a Twitter OAuth return request and store the account creds
 * in the DB. Redirects to user/%/edit/twitter
 *
 * @TODO Redirect better.
 *
 * I don't much like the use of drupal_goto() here as it might
 * interfere with other modules trying to piggyback on the form
 * submission, but setting $form['redirect'] was leaving us at the
 * twitter/oauth URL.
 */
function twitter_oauth_callback_form_submit(&$form, &$form_state) {
  $key = variable_get('twitter_consumer_key', '');
  $secret = variable_get('twitter_consumer_secret', '');
  $response = $form_state['twitter_oauth']['response'];

  $twitter = new TwitterOAuth($key, $secret, $response['oauth_token'], $response['oauth_token_secret']);
  $twitter_account = $twitter->users_show($response['screen_name']);
  $twitter_account->set_auth($response);
  $account = $form_state['twitter_oauth']['account'];
  twitter_account_save($twitter_account, TRUE, $account);

  $form['#programmed'] = FALSE;

  $form_state['redirect'] = url('user/' . $account->uid . '/edit/twitter');
  // redirect isn't firing - because we're using drupal_submit_form()?
  // - so adding drupal_goto() here (but not liking it).
  drupal_goto('user/' . $account->uid . '/edit/twitter');
}
