<?php

/**
 * Copyright (C) 2014 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

require_once "openmediavault/config.inc";
require_once "openmediavault/error.inc";
require_once "openmediavault/initscript.inc";
require_once "openmediavault/log.inc";
require_once "openmediavault/module.inc";
require_once "openmediavault/util.inc";

class OMVModuleOpenVpn extends OMVModuleServiceAbstract implements
    OMVINotifyListener,
    OMVIModuleServiceStatus
{
    /* Helpers */

    private function getEventMessagePath()
    {
        return "org.openmediavault.services.openvpn";
    }

    private function getXPath()
    {
        return "/config/services/openvpn";
    }

    /* /Helpers */

    /**
     * Get the module name.
     */
    public function getName()
    {
        return "openvpn";
    }

    /**
     * Get the status of the service
     */
    public function getStatus()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getXPath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        $initd = new OMVSysVInitScript($this->getName());

        return array(
            "name"    => $this->getName(),
            "title"   => gettext("OpenVPN"),
            "enabled" => boolval($object["enable"]),
            "running" => $initd->status() == 0
        );
    }

    /**
     * Create and update the configuration
     */
    public function applyConfig()
    {
        $cmd = sprintf("omv-mkconf %s setup 2>&1", $this->getName());

        if (0 !== $this->exec($cmd, $output)) {
            throw new OMVException(
                OMVErrorMsg::E_EXEC_FAILED,
                $cmd,
                implode("\n", $output)
            );
        }
    }

    /**
     * Start the service
     */
    public function startService()
    {
        global $xmlConfig;

        $object = $xmlConfig->get($this->getXPath());

        if (is_null($object)) {
            throw new OMVException(
                OMVErrorMsg::E_CONFIG_GET_OBJECT_FAILED,
                $this->getXPath()
            );
        }

        if (!$object["enable"]) {
            return;
        }

        $initd = new OMVSysVInitScript($this->getName(), true);
        $initd->exec();
    }

    /**
     * Stop the service
     */
    public function stopService()
    {
        $initd = new OMVSysVInitScript($this->getName());
        $initd->exec();
    }

    /**
     * Bind listeners.
     */
    public function bindListeners(OMVNotifyDispatcher $dispatcher)
    {
        $dispatcher->addListener(
            OMV_NOTIFY_CREATE | OMV_NOTIFY_MODIFY | OMV_NOTIFY_DELETE,
            $this->getEventMessagePath(),
            array($this, "setDirty")
        );
    }
}

// Register module
$moduleMgr = &OMVModuleMgr::getInstance();
$moduleMgr->registerModule(new OMVModuleOpenVpn());

// Register new logfile type
OMVLogFile::registerType(
    "openvpn",
    array(
        "filename" => "openvpn",
        "filepath" => "/var/log/openvpn.log",
        "regex" => "/^(\w+\s\w+\s\d+\s+\d+:\d+:\d+\s\d+)\s+(.*)$/",
        "columns" => array(
            "ts" => array(
                "index" => 1,
                "func" => create_function(
                    "\$v",
                    "return strpdate(\$v, \"D M d H:i:s Y\");"
                )
            ),
            "date" => array(
                "index" => 1,
                "func" => create_function(
                    "\$v",
                    "return strftime(\"%c\", strpdate(\$v,\"D M d H:i:s Y\"));"
                )
            ),
            "event" => 2
        )
    )
);
